#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

const char* CRLF = "\r\n";

// Request line = Method SP Request-URI SP HTTP-Version CRLF    
// SP = space

typedef struct {
    const char* data;
    size_t len;
} string;

bool stringsEqual(const string* l, const string* r) {
    size_t minLen = l->len < r->len ? l->len : r->len;
    return l->len == r->len && memcmp(l->data, r->data, l->len) == 0;
}

string stringFromCstr(const char* str) {
    string s;
    s.len = strlen(str);
    s.data = str;
    return s;
}

void stringTrimSpaces(string* s){
    // left trim;
    while (*s->data == ' '){
        s->data += 1;
    }

    // right trim
    while (s->len > 0 && s->data[s->len - 1] == ' '){
        s->len -= 1;
    }
}

typedef struct {
    string method;
    string uri;
    string version;
} httpReqLine;

typedef enum {
    HTTP_RES_OK = 200,    
    HTTP_RES_BAD_REQUEST = 500,
    HTTP_RES_INTERNAL_SERVER_ERR = 400,

} httpResult;

httpReqLine httpReqLineInit() {
    httpReqLine line;
    memset(&line, 0, sizeof(line));
    return line;
}

typedef struct {
    char* start;
    size_t len;
} stringView;

typedef struct {
    stringView* splits; // array of stringViews
    size_t count;
    size_t capacity;
} stringSplit;

static stringSplit splitString(const char* string, size_t len, const char* delim){
    stringSplit res;
    const char* start = string;
    size_t res_i = 0;
    size_t delimLen = strlen(delim);

    res.capacity = 8;
    res.splits = calloc(res.capacity, sizeof(stringView));
    res.count = 0;

    for (size_t i = 0; i < len; ++i){
        if (i + delimLen < len && memcmp(&string[i], delim, delimLen) == 0){
            res.splits[res_i].start = start;
            res.splits[res_i].len = &string[i] - start;
            res.count += 1;
            res_i += 1;
            start = &string[i + delimLen];
            i += delimLen;

            if (res.count == res.capacity){
                res.capacity += 2;
                stringView* temp = realloc(res.splits, sizeof(stringView) * res.capacity);
                if (temp) {
                    res.splits = temp;
                } else {
                    perror("realloc()");
                    abort();
                }
            }
        }
    }
    size_t lastLen = &string[len] - start;
    if (lastLen > 0){
        res.splits[res_i].start = start;
        res.splits[res_i].len = lastLen;
        res.count += 1;
    }
    return res;
}

static void freeSplits(stringSplit* splits){
    if (splits){
        free(splits->splits);
        splits->splits = NULL;
    }
}

httpResult parseReqLine(httpReqLine* reqLine, const char* buffer, size_t len){
    char* split;
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

int cilentHandler(int cilentSocket){
    ssize_t num = 0;
    char buffer[1024];
    char fileBuffer[1024];
    size_t byteRead = 0;
    const char* header = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n";
    const char* hello = "HTTP/1.0 200 OK\r\n\r\n<h1>Hello World!</h1>";
    const char* bye = "HTTP/1.0 200 OK\r\n\r\n<h1>Bye World!</h1>";
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
        httpResult res = parseReqLine(&reqLine, lines.splits[0].start, lines.splits[0].len);
        if (res != HTTP_RES_OK){
            // return correct error and error page
            printf("ERROR: failed to parse request line\n");
            return -1;
        }

        string routeHello = stringFromCstr("/hello");
        string routeBye = stringFromCstr("/bye");

        if (stringsEqual(&reqLine.uri, &routeHello)){
            (void)write(cilentSocket, hello, strlen(hello));
        }

        if (stringsEqual(&reqLine.uri, &routeBye)){
            (void)write(cilentSocket, bye, strlen(bye));
        } else {
            printf("ERROR: unknown route: \"%.*s\"\n", (int)reqLine.uri.len, reqLine.uri.data);
            return -1;
        }


        (void)write(cilentSocket, header, strlen(header));
        // Used to write the html file
        while ((byteRead = fread(fileBuffer, 1, sizeof(fileBuffer), index_file)) > 0){ 
            (void)write(cilentSocket, fileBuffer, byteRead);
        }

        fclose(index_file);
        freeSplits(&lines);
        close(cilentSocket);
        break;
    }
    printf("\n---\n");

    return 0;
}


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
    
    sockAddr.sin_port = htons(5050); // Local host 
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
    printf("listen success \n");

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