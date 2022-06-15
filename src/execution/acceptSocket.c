#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "assert.h"
#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "errno.h"
#include "string.h"

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    struct sockaddr_in address;
    bzero(&address, sizeof(address));

    address.sin_family = AF_INET;
    // transfer ip into address
    inet_pton(AF_INET, ip, &address.sin_addr);
    // change byte order from host to network
    address.sin_port = htons(port);

    // ipv4 sockedt , PF_INET and Sock_STREAM means TCP , 0 means default protocal
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock > 0);

    // bind socket instance with ip address
    int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    
    // during this time, we can shut up the client and disconnet the Established connection 
    sleep(20);

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&address, &client_addrlength);
    if (connfd < 0) {
        printf("errno is: %d\n", errno);
    } else {
        char remote[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN);
        int accept_port = ntohs(client.sin_port);
        printf("connected with ip: %s and port: %d\n", remote, accept_port);
        close(connfd);
    }
    close(sock);
    return 0;
    

}