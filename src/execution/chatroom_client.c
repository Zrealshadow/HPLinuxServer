#define _GNU_SOURCE 1
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "stdio.h"
#include "unistd.h"
#include "errno.h"
#include "string.h"
#include "fcntl.h"
#include "stdlib.h"
#include "assert.h"
#include "poll.h"

#define BUFFER_SIZE 64

int main(int argc, char const *argv[])
{
        /* code */
    if(argc <= 2){
        return 1;
    }
    const char * ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip,&server_addr.sin_addr);
    server_addr.sin_port = htons(port);
    
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >=0);

    int ret = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret < 0){
        printf("Connection failed!\n");
        close(sockfd);
        return 1;
    }

    struct pollfd fds[2];

    // 0 file descriptor represent standard input 
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;

    char read_buf[BUFFER_SIZE];
    int pipefd[2];
    ret = pipe(pipefd);
    assert(ret != -1);

    while(1){
        ret = poll(fds,2,-1);
        if(ret < 0){
            printf("poll failure \n");
            break;
        }

        if(fds[1].revents & POLLRDHUP) {
            // server close
            printf("server close the connection\n");
            break;
        } else if(fds[1].revents & POLLIN){
            // trigger input
            memset(read_buf, '\0', BUFFER_SIZE);
            recv(fds[1].fd, read_buf, BUFFER_SIZE - 1, 0);
            printf("%s\n",read_buf);
        }

        if(fds[0].revents & POLLIN){
            // write input
            // Use Pipeline and splice to move message from standard input to socked file
            // zero copy
            ret = splice(0, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
            ret = splice(pipefd[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        }
    }
    close(sockfd);
    return 0;
}

