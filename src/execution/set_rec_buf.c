#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "assert.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "stdlib.h"
#include "errno.h"

#define BUFFER_SIZE 1024

int main(int argc, char const *argv[])
{
    if (argc <= 2)
    {
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_address.sin_addr);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int recvbuf = atoi(argv[3]);
    int len = sizeof(recvbuf);

    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf));
    getsockopt(sockfd,SOL_SOCKET,SO_RCVBUF, &recvbuf, (socklen_t*)&len);
    printf("the tcp receive buffer size after setting is %d\n", recvbuf);

    int ret = bind(sockfd, (struct sockaddr*) &server_address, sizeof(server_address));
    assert(ret != -1);

    ret = listen(sockfd, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sockfd, (struct sockaddr*)&client, &client_addrlength);
    if (connfd < 0){
        printf("errno is %d\n", errno);
    } else {
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE);
        while(recv(connfd, buffer, BUFFER_SIZE - 1,0) >0){}
        close(connfd);
    }
    close(sockfd);
    return 0;
}
