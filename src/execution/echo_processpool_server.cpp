#include "process_pool.hpp"

class echo_conn
{
private:
    /* data */
public:
    echo_conn(/* args */){};
    ~echo_conn(){};

    void init(fd_t epollfd, fd_t sockfd, const sockaddr_in &client_addr);
    int process();

private:
    static const int BUFFER_SIZE = 1024;
    fd_t m_epollfd;
    fd_t m_sockfd;
    sockaddr_in m_address;
    char m_buf[BUFFER_SIZE];
    int m_read_idx;
};

// fd_t echo_conn::m_epollfd = -1;

void echo_conn::init(fd_t epollfd, fd_t sockfd, const sockaddr_in &client_addr)
{
    m_epollfd = epollfd;
    m_sockfd = sockfd;
    m_address = client_addr;
    memset(m_buf, '\0', BUFFER_SIZE);
    m_read_idx = 0;
}

int echo_conn::process()
{
    int idx = 0;
    int ret = -1;
    memset(m_buf, '\0', BUFFER_SIZE);
    // while (true)

    // idx = m_read_idx;
    // ret = recv(m_sockfd, m_buf + m_read_idx, BUFFER_SIZE - 1 - idx, 0);
    ret = recv(m_sockfd, m_buf, BUFFER_SIZE - 1, 0);
    if (ret < 0)
    {
        if (errno != EAGAIN)
        {
            // conn is closed
            removefd(m_epollfd, m_sockfd);
            return 0;
        }
        printf("ret < 0\n");

        // break;
    }
    else if (ret == 0)
    { // conn is closed
        removefd(m_epollfd, m_sockfd);
        printf("ret = 0 connection is closed by client\n");
        return 0;
        // break;
    }
    else
    {
        // ret <= 0, means connfd closed by client or some failure happened
        // ret > 0, some data input
        m_read_idx += ret;
        printf("user content is : %s\n", m_buf);
        for (; idx < m_read_idx; ++idx)
        {
            if ((idx >= 1) && (m_buf[idx - 1] == '\r') && (m_buf[idx] == '\n'))
            {
                break;
            }
        }

        if (idx == m_read_idx)
        {
            // need read more data
            // continue;
        }
        // change '\r'  to '\0'
        // before '\r' is a string of file path
        m_buf[idx - 1] = '\0';
        char *file_name = m_buf;

        // echo this msg
        send(m_sockfd, m_buf, BUFFER_SIZE, 0);
    }
    return 1;
}

int main(int argc, char const *argv[])
{
    if (argc <= 2)
    {
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    int ret = bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    ret = listen(listenfd, 5);
    assert(ret != -1);
    processpool<echo_conn> *pool = processpool<echo_conn>::create(listenfd, 2);
    if (pool)
    {
        pool->run();
        delete pool;
    }
    close(listenfd);
    exit(0);
    return 0;
}
