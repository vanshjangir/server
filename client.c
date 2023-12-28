#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080


typedef struct server_info{
    int sockfd;
    struct sockaddr_in addr;
    socklen_t addr_len;
} server_info;

void* send_f(server_info *SERVER){
    while(1){
        char buf[1024];
        getchar();
        send(SERVER->sockfd, "00000028MSG:10000001:10000002:HELLO;", 36, 0);
    }
}

void* recv_f(server_info *SERVER){
    while(1){
        char header[9];
        recv(SERVER->sockfd, header, 8, 0);
        char len = atoi(header);
        char buf[len +1];
        recv(SERVER->sockfd, buf, len, 0);
        buf[len] = '\0';
        printf("%s\n", buf);
    }
}

int main(int argc, char **argv){

    struct server_info *SERVER;
    pthread_t send_t, recv_t;

    SERVER = (server_info*)malloc(sizeof(server_info));
    SERVER->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    SERVER->addr.sin_port = htons(SERVER_PORT);
    SERVER->addr.sin_family = AF_INET;
    SERVER->addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    SERVER->addr_len = sizeof(SERVER->addr);

    while(1){
        if(connect(SERVER->sockfd, (struct sockaddr*)&SERVER->addr, SERVER->addr_len) == 0){
            break;
        }
        printf("trying againg\n");
        sleep(1);
    }
    getchar();

    pthread_create(&send_t, NULL, (void*)&send_f, (void*) SERVER);
    pthread_create(&recv_t, NULL, (void*)&recv_f, (void*) SERVER);

    pthread_join(send_t, NULL);
    pthread_join(recv_t, NULL);
}
