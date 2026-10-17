#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#include "stringFuncs.h"
#include "file.h"

#define CRLF "\r\n"

const stringView WEB_ROOT = STRING_VIEW_FROM_LITERAL("./www/");
static stringView error404 = STRING_VIEW_FROM_LITERAL("<p>ERROR 404 Not Found</p><p><a href=\"/\">back to home</a></p>");

// Request line = Method Request-URI HTTP-Version CRLF    

typedef struct {
    stringView method;
    stringView uri;
    stringView version;
} httpReqLine;

typedef enum httpResponse {
    HTTP_RES_OK = 200,    
    HTTP_RES_BAD_REQUEST = 400,
    HTTP_RES_NOT_FOUND = 404,
    HTTP_RES_INTERNAL_SERVER_ERR = 500,

} httpResponse;

const char* httpResponseToString(httpResponse res) {
    switch (res){
        case HTTP_RES_OK:
            return "OK";
        case HTTP_RES_BAD_REQUEST:
            return "Bad Request";
        case HTTP_RES_INTERNAL_SERVER_ERR:
            return "Internal Server Error";
        case HTTP_RES_NOT_FOUND:
            return "Not Found";
        default:
            return "Unknown";
    }
}

typedef struct {
    const char* version;
    httpResponse statusCode;
} httpResponseLine;

httpReqLine httpReqLineInit() {
    httpReqLine line;
    memset(&line, 0, sizeof(line));
    return line;
}

httpResponse parseReqLine(httpReqLine* reqLine, const char* buffer, size_t len){
    if (!buffer || !reqLine){
        return HTTP_RES_INTERNAL_SERVER_ERR;
    }
    stringSplit tokens = splitString(buffer, len, " ");

    if (tokens.count != 3){
        printf("ERROR: invalid request line: expected 3 components, got %zu\n", tokens.count);
        return HTTP_RES_BAD_REQUEST;
    }

    reqLine->method.data = tokens.splits[0].data;
    reqLine->method.len = tokens.splits[0].len;
    reqLine->uri.data = tokens.splits[1].data;
    reqLine->uri.len = tokens.splits[1].len;
    reqLine->version.data = tokens.splits[2].data;
    reqLine->version.len = tokens.splits[2].len;


    freeSplits(&tokens);

    return HTTP_RES_OK;
}

stringView httpResponseGen(char* buffer, size_t bufferLen, httpResponse resp, size_t bodyLen){
    stringView response;
    response.len = 0;
    memset(buffer, 0, bufferLen);

    response.len += sprintf(buffer, "%s %d %s" CRLF, "HTTP/1.0", resp, httpResponseToString(resp));
    response.len += sprintf(buffer + response.len, "Content-Length: %zu" CRLF, bodyLen);
    response.len += sprintf(buffer + response.len, CRLF);
    response.data = buffer;
    return response;
}

bool httpSendResponse(int socket, stringView header, stringView body) {
    ssize_t n = send(socket, header.data, header.len, 0);
    if (n < 0) {
        perror("send()");
        return false;
    }
    if (n == 0){
        fprintf(stderr, "send() returned 0\n");
        return false;
    }
    n = send(socket, body.data, body.len, 0);
    return true;
}


bool httpServeFile(int socket, stringView fileName){
    char fileNameBuffer[PATH_MAX];
    char buffer[64];
    int inputFd = -1;
    off_t sentFileOffset = 0;
    size_t sent = 0;
    bool ret = true;
    ssize_t res = 0;
    stringView header;

    memset(fileNameBuffer, 0, sizeof(fileNameBuffer));
    memcpy(fileNameBuffer, WEB_ROOT.data, WEB_ROOT.len);
    memcpy(fileNameBuffer + WEB_ROOT.len - 1, fileName.data, fileName.len);

    fileData fileData = getFileData(stringFromCstr(fileNameBuffer));

    if (!fileData.exists){
        (void)httpSendResponse(socket, httpResponseGen(buffer, sizeof(buffer), HTTP_RES_NOT_FOUND, error404.len), error404);
        return false;
    }

    header = httpResponseGen(buffer, sizeof(buffer), HTTP_RES_OK, fileData.size);
    
    ssize_t n = send(socket, header.data, header.len, MSG_MORE);
    if (n < 0) {
        perror("send()");
        ret = false;
        goto cleanup;
    }
    if (n == 0){
        fprintf(stderr, "send() returned 0\n");
        ret = false;
        goto cleanup;
    }
    inputFd = open(fileNameBuffer, O_RDONLY);
    if (inputFd < 0){
        (void)httpSendResponse(socket, httpResponseGen(buffer, sizeof(buffer), HTTP_RES_NOT_FOUND, error404.len), error404);
        ret = false;
        goto cleanup;     
    }
    // sendfile(2) sends the file without using any userspace memory
    while (sent < fileData.size){
        res = sendfile(socket, inputFd, &sentFileOffset, fileData.size);
        if (res < 0){
            printf("ERROR sendFile() failed for \"%s\": %s\n", fileNameBuffer, strerror(errno));
            (void)httpSendResponse(socket, httpResponseGen(buffer, sizeof(buffer), HTTP_RES_INTERNAL_SERVER_ERR, error404.len), error404);
            ret = false;
            goto cleanup;              
        }
        sent += res;
    }

cleanup:
    if (inputFd != -1) {
        close(inputFd);        
    }
    return ret;
}

int cilentHandler(int cilentSocket){
    ssize_t num = 0;
    char buffer[1024];

    printf("\n---\n");
    while(1){
        memset(buffer, 0, sizeof(buffer));

        num = read(cilentSocket, buffer, sizeof(buffer) - 1);
        if (num < 0){
            perror("read()");
            return -1;
        }
        if (num == 0){
            printf("Connection closed! \n");
            break;
        }

        printf("RQUESTS:\n%s", buffer);

        stringSplit lines = splitString(buffer, num, CRLF);
        httpReqLine reqLine = httpReqLineInit();
        httpResponse res = parseReqLine(&reqLine, lines.splits[0].data, lines.splits[0].len);
        freeSplits(&lines);
        if (res != HTTP_RES_OK){
            // return correct error and error page
            printf("ERROR: failed to parse request line\n");
            return -1;
        }

        stringView root = stringFromCstr("/");

        if (stringsEqual(&reqLine.uri, &root)){
            if (!httpServeFile(cilentSocket, stringFromCstr("test.html"))){
                return -1;
            }
        } else {
            printf("ERROR: unknown route: \"%.*s\"\n", (int)reqLine.uri.len, reqLine.uri.data);
            httpSendResponse(                
                cilentSocket,
                httpResponseGen(buffer, sizeof(buffer), HTTP_RES_NOT_FOUND, error404.len), 
                error404);
            return -1;
        }
        close(cilentSocket);
        break;
    }
    printf("\n---\n");

    return 0;
}

const int PORT = 5050;

int main(void){
    int returnCode = 0;
    struct sockaddr_in sockAddr;
    int tcpSocket = 0;
    int ret = 0;
    int cilentSocket = 0;
    int enabled = true;

    fileData webRootData = getFileData(WEB_ROOT);
    if (!webRootData.exists){
        /* rwxr-xr-x*/
        mkdir(WEB_ROOT.data, S_IEXEC | S_IWRITE | S_IREAD | S_IRGRP | S_IXGRP | S_IXOTH | S_IROTH);
    }

    memset(&sockAddr, 0, sizeof(sockAddr));
    tcpSocket = socket(
        AF_INET, /* The IPv4 (domain)*/
        SOCK_STREAM, /* The TCP (type)*/
         0 /* Doesn't matter (protocol)*/
    );

    if (tcpSocket < 0) {
        perror("socket");
        return 1;
    }
    printf("Socket created! \n");

    (void)setsockopt(tcpSocket, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
    
    sockAddr.sin_port = htons(PORT); // Local host 
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;

    returnCode = bind(tcpSocket, (const struct sockaddr*)&sockAddr, sizeof(sockAddr));
    if (returnCode < 0){
        perror("bind()");
        ret = 1;
        goto exit;
    }
    printf("bind success \n");

    returnCode = listen(tcpSocket, SOMAXCONN); // SOMAXCONN because the support value of backlog doesn't really matter here
    if (returnCode < 0){
        perror("listen()");
        ret = 1;
        goto exit;
    }
    printf("listen on localhost:%d/\n", PORT);

    while(1){
        printf("waiting for connection... \n");
        cilentSocket = accept(tcpSocket, NULL, NULL);

        printf("connected! \n");
        returnCode = cilentHandler(cilentSocket);

    }

exit:
    close(tcpSocket);
    return ret;
}