# Scalable client manager
The idea is to handle multiple live, concurrent connections on a single machine with minimum latency in each connection. It does that by using thread pooling and [EPOLL](https://en.wikipedia.org/wiki/Epoll)

## Usage
The create_server function receive four arguments
1. A server_args struct
2. Maximum number of clients to be handled
3. Maximum numer of threads to be created

If a client is sending a message the function provided in server_args will be called (see example).

## How it works
All the file descriptors are added to the epoll structure. When a file descriptor has a new connection or some data to read, that fd is added in the ready state (by the kernel in the background). When the epoll_wait() is called, it returns the number of file descriptors that are in the ready state and the file descriptors are added in the epoll_arr. If the fd is s_socket then it means there is a new connection so a new client is added. If the fd is a client and there is data to read then a new task is added in the QUEUE. All the threads are waiting for a task to get. [Thundering Herd](https://en.wikipedia.org/wiki/Thundering_herd_problem) problem is solved by using mutex and conditional variables. If the fd is a client and it has lost the connection then the fd is removed from the epoll structure.

## Useful
1. [A million web socket connections](https://www.youtube.com/watch?v=LI1YTFMi8W4)
2. [Fix EPOLL](https://idea.popcount.org/2017-02-20-epoll-is-fundamentally-broken-12/)
