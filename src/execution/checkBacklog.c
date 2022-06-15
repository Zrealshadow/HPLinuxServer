#include "stdio.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "signal.h"
#include "unistd.h"
#include "stdlib.h"
#include "assert.h"
#include "string.h"
#include "stdbool.h"

static bool stop = false;

static void handle_term(int sig)
{
    stop = true;
}

int main(int argc, char *argv[])
{
    signal(SIGTERM, handle_term);

    if (argc <= 3)
    {
        // printf("usage: %s ip_address port_number backlog\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);

    // ipv4 socked, TPC, default protocal
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    // create IPV4 address
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    // change ip string to sin_addr (bytes)
    inet_pton(AF_INET, ip, &address.sin_addr);

    // bind ip with sock file
    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, backlog);

    while(!stop){
        sleep(1);
    }

    close(sock);
    return 0;
}
