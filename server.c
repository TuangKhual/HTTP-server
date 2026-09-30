#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

int cilentHandler(int cilentSocket){
    ssize_t num = 0;
    char buffer[1024];
    const char* hello = "HTTP/1.0 200 OK\r\n\r\n<h1>Hello World!</h1>";

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
        (void)write(cilentSocket, hello, strlen(hello));
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