#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "assert.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "stdlib.h"
#include "errno.h"

#define BUFFER_SIZE 512

int main(int argc, char const *argv[])
{
    if (argc <= 2)
    {
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int sendbuf = atoi(argv[3]);
    socklen_t len = sizeof(sendbuf);
    // setsockopt(sock, SOL_)
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuf, len);
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuf, &len);

    printf("the tcp send buffer size after setting is %d\n", sendbuf);

    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) != -1)
    {
        char buffer[BUFFER_SIZE];
        memset(buffer, 'a', BUFFER_SIZE);
        send(sock, buffer, BUFFER_SIZE,0);
    }
    close(sock);
    return 0;
}
