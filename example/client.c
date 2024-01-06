#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER_IP "192.168.0.110"
#define SERVER_PORT 8080

typedef struct server_info{
    int sockfd;
    struct sockaddr_in addr;
    socklen_t addr_len;
} server_info;

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
    sleep(6);
    send(SERVER->sockfd, "00000028MSG:10000001:10000002:HELLO;", 36, 0);
    sleep(1000);
}
