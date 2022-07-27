# High Performance Linux Server

>This repo contains the code about the book << High Performance Linux Server Coding>>
>
>Lingze

## Usage

Compile Command

```shell
$ git clone https://github.com/Zrealshadow/HPLinuxServer.git
$ cd HPLinuxServer
$ mkdir bin build
$ cd build
$ cmake ..
$ cd ..
$ cmake --build ./build --config Debug --target all --
```

Now, all execution files are under `bin` directory.

For how to run these execution files, please check the source file 

---

### Reuse Code

There are some template which can be reused in linux sever development

**Time-out Server**

related file

- `src/execution/timeout_server.cpp`
- `src/include/lst_timer.h`
- `src/include/time_heap.h`
- `src/include/time_wheel_timer.h`

The main logic is that every slot time, the main process will receive a alarm signal. Then, the server will check all registered events whether they expires. If expires, the event will be delete in epoll table.

`time_heap` and `time_wheel_timer` are two more accurate timer, which support more fine-granularity slot-time.



**Server based on process pool**

related file

- `src/execution/echo_processpool_server.cpp`
- `src/include/process_pool.hpp`

Actually, this section is hard to reuse. The process pool is hard to be decoupled from the server logic. However, it is a good code example to peep the multi-process programming in C++. How to use `fork()` to create new process and how to use `pipeline` to exchange information between parent process and child process.



**Server based on thread pool**

related file

- `src/httpConn/http_conn.cpp`
- `src/httpConn/http_conn.h`
- `src/httpConn/web_server.cpp`
- `src/include/thread_pool.hpp`

In this code example, the `thread_pool` library is fully decoupled from the server logic. The only requirement is that the task unit should implement `process` method.

In `http_conn` , we use a Finite Automatic State Machine to parse http request. we can learn some template from this structure.



**Library for mult-thread communication**\

related file

- `src/include/locker.h`

Three class `sem` , `locker`, `cond` in this file can be reused.

### Note in Server Development

Two effective event processing pattern: Reactor  and Proactor

**Reactor**

The main process is the IO processing unit. It doesn't do any logic processing. The only work the main process do is to listen whether there is some events in file descriptor. If some event happens, the main process will assign it to the sub-process to handle event.

Take `epoll_wait` as an example, describe how Reactor works

- Main thread registers EPOLLIN sockfd in epollfd
- Main thread invokes `epoll_wait` to monitor any input data in sockfd
- If any data is entered, epoll_wait will inform main thread, and the main thread will put this reading event into request queue.
- The sub thread is waken up, read data from sockfd and process the request. Then  register writing-ready event in epollfd
- epoll_wait inform main thread of the writing-reading event, and the main thread will put this writing-reading event into request queue.
- Certain sub thread is waken up, get the event from request queue and write data into sockfd.

**Proactor**

The main different from **Reactor Pattern** is that all IO operators are handed over to the main thread and the linux kernel. Working thread is responsible for busniess logic. For every IO operator, we provide the buffer to linux kernel. When kernel finished IO operator, it will inform thread that the work is done.

