#if !defined(PROCESS_POOL_H)
#define PROCESS_POOL_H

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
#include "sys/wait.h"
#include "sys/stat.h"
#include "sys/epoll.h"
#include "signal.h"

typedef int fd_t;
class process
{
public:
    process() : m_pid(-1) {}

public:
    pid_t m_pid;     // sub process Id
    int m_pipefd[2]; // the pipeline between subprocess and main process
};

template <typename T>
class processpool
{
private:
    // private construct func, the default size of process pool is 8
    processpool(fd_t listenfd, int process_number = 8);

public:
    static processpool<T> *create(fd_t listenfd, int process_number = 8)
    {
        if (m_instance == nullptr)
        {
            m_instance = new processpool<T>(listenfd, process_number);
        }
        return m_instance;
    }

    ~processpool()
    {
        delete[] m_sub_process;
    }
    void run();

private:
    void setup_sig_pipe();
    void run_parent();
    void run_child();

private:
    static const int MAX_PROCESS_NUMBER = 16;
    static const int USER_PER_PROCESS = 0xffff;
    static const int MAX_EVENT_NUMBER = 10000;

    int m_process_number;
    int m_idx;
    fd_t m_epollfd;
    fd_t m_listenfd;
    bool m_stop;
    int *m_used_process;
    static processpool<T> *m_instance;
    process *m_sub_process;
};

template <typename T>
processpool<T> *processpool<T>::m_instance = NULL;

// the pipeline for signal to ctl main process
static int sig_pipefd[2];

static int setnonblocking(fd_t fd)
{
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

static void addfd(fd_t epollfd, fd_t fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

static void removefd(fd_t epollfd, fd_t fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

// callback to add signal to signal_pipeline
static void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], (char *)&sig, 1, 0);
    errno = save_errno;
}

static void addsig(int sig, void(handler)(int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

// the definetion of construct
template <typename T>
processpool<T>::processpool(fd_t listenfd, int process_number) : m_listenfd(listenfd),
                                                                 m_process_number(process_number), m_idx(-1), m_stop(false)
{
    assert((process_number > 0) && (process_number <= MAX_PROCESS_NUMBER));
    m_sub_process = new process[m_process_number];
    m_used_process = new int[m_process_number];
    assert(m_sub_process);

    for (int i = 0; i < process_number; i++)
    {
        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd);
        assert(ret == 0);

        m_sub_process[i].m_pid = fork();
        // sub process pid = 0; parent process pid  > 0;
        assert(m_sub_process[i].m_pid >= 0);

        if (m_sub_process[i].m_pid == 0)
        {
            // sub_process
            close(m_sub_process[i].m_pipefd[0]);
            m_idx = i;
            break;
        }
        else
        {
            // main prcoss
            close(m_sub_process[i].m_pipefd[1]);
            addfd(m_epollfd, m_sub_process[i].m_pipefd[0]);
            continue;
        }
    }
}

// initialize signal pipeline
template <typename T>
void processpool<T>::setup_sig_pipe()
{
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    assert(ret != -1);

    setnonblocking(sig_pipefd[1]);
    addfd(m_epollfd, sig_pipefd[0]);

    // add signal
    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGPIPE, SIG_IGN);
}

template <typename T>
void processpool<T>::run()
{
    if (m_idx != -1)
    {
        run_child();
        return;
    }
    run_parent();
}

template <typename T>
void processpool<T>::run_child()
{
    // setup the unified signal receiver
    setup_sig_pipe();

    fd_t pipefd = m_sub_process[m_idx].m_pipefd[1];
    addfd(m_epollfd, pipefd);

    epoll_event events[MAX_EVENT_NUMBER];
    T *users = new T[USER_PER_PROCESS];
    assert(users);
    int number = 0;
    int ret = -1;
    while (!m_stop)
    {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR))
        {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            fd_t sockfd = events[i].data.fd;
            if ((sockfd == pipefd) && (events[i].events & EPOLLIN))
            {
                // something from main process through pipeline
                // subprocess should accept the conn
                int client = 0;
                ret = recv(sockfd, (char *)&client, sizeof(client), 0);
                if (((ret < 0) && (errno != EAGAIN)) || ret == 0)
                {
                    continue;
                }
                else
                {
                    struct sockaddr_in client_address;
                    socklen_t client_addrlength = sizeof(client_address);
                    fd_t connfd = accept(m_listenfd, (struct sockaddr *)&client_address,
                                         &client_addrlength);
                    if (connfd < 0)
                    {
                        printf("errno is : %d\n", errno);
                        continue;
                    }
                    addfd(m_epollfd, connfd);
                    // classtype T should complete this interface (init)
                    users[connfd].init(m_epollfd, connfd, client_address);
                }
            }
            else if ((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                // receive the signal
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if (ret <= 0)
                {
                    continue;
                }
                else
                {
                    for (int i = 0; i < ret; i++)
                    {
                        switch (signals[i])
                        {
                        case SIGCHLD:
                        {
                            pid_t pid;
                            int stat;
                            while ((pid == waitpid(-1, &stat, WNOHANG)) > 0)
                            {
                                continue;
                            }
                            break;
                        }

                        case SIGTERM:
                        case SIGINT:
                        {
                            // stop process
                            printf("Kill child Process\n");
                            m_stop = true;
                            break;
                        }
                        default:
                        {
                            break;
                        }
                        }
                    }
                }
            }
            else if (events[i].events & EPOLLIN)
            {
                // if some data input from conn
                // T classtype process the logic
                ret = users[sockfd].process();
                if (ret == 0)
                {
                    printf("Send to pipefd")
                    // this process can be add another connection
                    send(m_sub_process[i].m_pipefd[1], (char *)&m_idx, sizeof(m_idx), 0);
                }
            }
            else
            {
                printf("Other Event Trigger in child process\n");
                continue;
            }
        }
    }

    // stop the sub process
    delete[] users;
    users = NULL;
    close(pipefd);
    close(m_epollfd);
}

template <typename T>
void processpool<T>::run_parent()
{
    setup_sig_pipe();

    addfd(m_epollfd, m_listenfd);
    epoll_event events[MAX_EVENT_NUMBER];
    int sub_process_count = 0;
    int new_conn = 1;
    int number = 0;
    int ret = -1;

    while (!m_stop)
    {
        number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR))
        {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; i++)
        {
            fd_t sockfd = events[i].data.fd;

            if (sockfd == m_listenfd)
            {
                // new connection build
                int i = sub_process_count;
                // choose a sub process, inform it of the coming of new client
                do
                {
                    if (m_sub_process[i].m_pid != -1 && m_used_process[i] != 1)
                    {
                        // if pid == -1, means this process is out or is working
                        // we can assign this new client to this sub process
                        break;
                    }
                    i = (i + 1) % m_process_number;
                } while (i != sub_process_count);
                // stop if no free process in a whole round

                if (m_sub_process[i].m_pid == -1 || m_used_process[i] == 1)
                {
                    struct sockaddr_in client_address;
                    socklen_t client_addrlength = sizeof(client_address);
                    int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
                    if (connfd < 0)
                    {
                        printf("errno is: %d\n", errno);
                        continue;
                    }
                    const char *info = "too many users\n";
                    printf("%s", info);
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;
                }

                // i process is selected as the assigned process
                sub_process_count = (i + 1) % m_process_number;
                m_used_process[i] = 1;

                // don't understand the meaning of new_conn, it's seems like a struct{}{} in go
                // just inform the subprocess that you should accept a new connection from listenfd
                send(m_sub_process[i].m_pipefd[0], (char *)&new_conn, sizeof(new_conn), 0);

                printf("send request to child %d pid %d\n", i, m_sub_process[i].m_pid);
            }
            else if ((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if (ret < 0)
                {
                    continue;
                }
                else
                {
                    for (int i = 0; i < ret; i++)
                    {
                        switch (signals[i])
                        {
                        case SIGCHLD:
                        {
                            pid_t pid;
                            int stat;
                            printf("Get SIGCHILD Signal\n");
                            // while ((pid == waitpid(-1, &stat, WNOHANG)) > 0)
                            for (; true;)
                            {
                                pid = wait(&stat);
                                if (pid == -1)
                                {
                                    break;
                                }

                                printf("Child PID %d Join\n", pid);
                                // reclaim the sub process
                                for (int i = 0; i < m_process_number; ++i)
                                {
                                    if (m_sub_process[i].m_pid == pid)
                                    {
                                        printf("[!!!!] Close the Resouce of Child PID %d\n", pid);
                                        close(m_sub_process[i].m_pipefd[0]);
                                        m_sub_process[i].m_pid = -1;
                                    }
                                }
                            }

                            // if all subprocess is join
                            // main process finishes either
                            printf("WAIT Failure pid %d\n", pid);
                            m_stop = true;
                            for (int i = 0; i < m_process_number; ++i)
                            {
                                if (m_sub_process[i].m_pid != -1)
                                {
                                    printf("idx %d pid != -1\n", i);
                                    m_stop = false;
                                }
                            }
                            break;
                        }

                        case SIGTERM:
                        case SIGINT:
                        {
                            printf("kill all child process now\n");
                            for (int i = 0; i < m_process_number; ++i)
                            {
                                int pid = m_sub_process[i].m_pid;
                                if (pid != -1)
                                {
                                    kill(pid, SIGTERM);
                                }
                            }
                            break;
                        }
                        default:
                        {
                            break;
                        }
                        }
                    }
                }
            }
            else if (events[i].events & EPOLLIN)
            {
                // find the true
                for (int i = 0; i < m_process_number; i++)
                {
                    if (events[i].data.fd == m_sub_process[i].m_pipefd[0])
                    {
                        printf("Process %d can be used again\n", i);
                        m_used_process[i] = 0;
                        break;
                    }
                }
            }
            else
            {
                // other events, skip
                continue;
            }
        }
    }
    close(m_epollfd);
}

#endif // PROCESS_POOL_H