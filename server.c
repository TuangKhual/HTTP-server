#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include "stringFuncs.h"

#define CRLF "\r\n"

// Request line = Method Request-URI HTTP-Version CRLF    

typedef struct {
    string method;
    string uri;
    string version;
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

    reqLine->method.data = tokens.splits[0].start;
    reqLine->method.len = tokens.splits[0].len;
    reqLine->uri.data = tokens.splits[1].start;
    reqLine->uri.len = tokens.splits[1].len;
    reqLine->version.data = tokens.splits[2].start;
    reqLine->version.len = tokens.splits[2].len;


    freeSplits(&tokens);

    return HTTP_RES_OK;
}

string httpResponseGen(char* buffer, size_t bufferLen, httpResponse resp, size_t bodyLen){
    string response;
    response.len = 0;
    memset(buffer, 0, bufferLen);

    response.len += sprintf(buffer, "%s %d %s" CRLF, "HTTP/1.0", resp, httpResponseToString(resp));
    response.len += sprintf(buffer + response.len, "Content-Length: %zu" CRLF, bodyLen);
    response.len += sprintf(buffer + response.len, CRLF);
    response.data = buffer;
    return response;
}

bool httpSendResponse(int socket, string header, string body) {
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

int cilentHandler(int cilentSocket){
    ssize_t num = 0;
    char buffer[1024];
    char fileBuffer[1024];
    size_t byteRead = 0;
    const char* header = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n";
    string helloBody = stringFromCstr("<h1>Hello World!</h1>");
    string byeBody = stringFromCstr("<h1>Bye World!</h1>");
    string error404 = stringFromCstr("<p>ERROR 404 Not Found</p><p><a href=\"/\">back to home</a></p>");
    FILE *index_file;

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
        index_file = fopen("html-test.html", "r");
        if (index_file == NULL){
            perror("fopen()");
            return -1;
        }

        printf("RQUESTS:\n%s", buffer);

        stringSplit lines = splitString(buffer, num, CRLF);
        httpReqLine reqLine = httpReqLineInit();
        httpResponse res = parseReqLine(&reqLine, lines.splits[0].start, lines.splits[0].len);
        freeSplits(&lines);
        if (res != HTTP_RES_OK){
            // return correct error and error page
            printf("ERROR: failed to parse request line\n");
            return -1;
        }

        string routeHello = stringFromCstr("/hello");
        string routeBye = stringFromCstr("/bye");

        if (stringsEqual(&reqLine.uri, &routeHello)){
            if (!httpSendResponse(
                cilentSocket,
                httpResponseGen(buffer, sizeof(buffer), HTTP_RES_OK, helloBody.len), 
                helloBody)) return -1;
        }
        else if (stringsEqual(&reqLine.uri, &routeBye)){
            if (!httpSendResponse(                
                cilentSocket,
                httpResponseGen(buffer, sizeof(buffer), HTTP_RES_OK, byeBody.len), 
                byeBody)) return -1;
        } else {
            printf("ERROR: unknown route: \"%.*s\"\n", (int)reqLine.uri.len, reqLine.uri.data);
            httpSendResponse(                
                cilentSocket,
                httpResponseGen(buffer, sizeof(buffer), HTTP_RES_NOT_FOUND, error404.len), 
                error404);
            return -1;
        }

/*   reimplement later
        (void)write(cilentSocket, header, strlen(header));
        // Used to write the html file
        while ((byteRead = fread(fileBuffer, 1, sizeof(fileBuffer), index_file)) > 0){ 
            (void)write(cilentSocket, fileBuffer, byteRead);
        }
        fclose(index_file);
*/
        close(cilentSocket);
        break;
    }
    printf("\n---\n");

    return 0;
}

const int PORT = 5050;


int main(void){
    /* declare */
    int returnCode = 0;
    struct sockaddr_in sockAddr;
    int tcpSocket = 0;
    int ret = 0;
    int cilentSocket = 0;
    int enabled = true;

    /* initialize */
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