#ifndef SCM_H
#define SCM_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <unistd.h>

#define SERVER_PORT 8080

typedef struct{
    int sockfd;
    struct sockaddr_in addr;
    socklen_t addr_len;
} client_info;

typedef struct NODE{
    int fd;
    struct NODE *link; 
} NODE;

typedef struct{
    NODE *head;
    NODE *rear;
    int num_nodes;
} threadqueue;

typedef struct{
    void* (*fptr)(int,void*);
    void *args;
    int fd;
} server_args;

enum state{
    STATEFUL,
    STATELESS,
};

int create_server(server_args*,int,int,enum state);
NODE* get_node(int);
void enqueue(int);
NODE* dequeue();
void* thread_f(server_args*);

#endif // !SCM_H
