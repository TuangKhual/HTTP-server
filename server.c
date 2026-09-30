#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>


int main(void){
    /* declare */
    int returnCode = 0;
    struct sockaddr_in sockAddr;
    int tcpSocket = 0;
    int ret = 0;
    int cilentSocket = 0;

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
    
    sockAddr.sin_port = htons(5050);
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

    printf("waiting for connection... \n");
    cilentSocket = accept(tcpSocket, NULL, NULL);

    printf("connected! \n");

exit:
    close(tcpSocket);
    return ret;
}