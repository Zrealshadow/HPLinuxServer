#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#pragma once

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/uio.h>
#include "locker.h"

class http_conn
{

public:
    /* filename length */
    static const int FILENAME_LEN = 200;
    /* size of read buffer */
    static const int READ_BUFFER_SIZE = 2048;
    /* size of write buffer */
    static const int WRITE_BUFFER_SIZE = 1024;

    /* the method of http, (we only support GET currently)*/
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATCH
    };
    /* the state of state machine*/
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };

    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSE_CONNECTION
    };

    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn(){};
    ~http_conn(){};

public:
    void init(int sockfd, const sockaddr_in &addr);

    void close_conn(bool real_close = true);

    void process();

    bool read();

    bool write();

private:
    void init();

    // parse the http request
    HTTP_CODE process_read();

    // fill http response
    bool process_write(HTTP_CODE ret);

    // helper func for process_read
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();

    char *get_line() {return  m_read_buf + m_start_line;}

    LINE_STATUS parse_line();

    // helper func for prcess_write
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;
    static int m_user_count;

private:
    int m_sockfd;
    sockaddr_in m_address;
    char m_read_buf[READ_BUFFER_SIZE];
    
    // indicate the next position in the buffer 
    // where the last byte of client data has been read
    int m_read_idx;

    // the position of byte which are currently parsed
    int m_checked_idx;

    // the index of line which are currently parsed
    int m_start_line;

    char m_write_buf[WRITE_BUFFER_SIZE];

    // the number of bytes which are waited for sending
    int m_write_idx;

    CHECK_STATE m_check_state;

    METHOD m_method;

    // the path of requested file
    char m_real_file[FILENAME_LEN];

    // the name of file
    char * m_url;

    // the version of http , only support http 1.1
    char * m_version;

    // the name of host
    char * m_host;

    int m_content_length;
    
    // whether to keep connection in http
    bool m_linger;

    // the start address of mmap file
    char * m_file_address;

    struct stat m_file_state;

    struct iovec m_iv[2];
    
    int m_iv_count;
};

#endif