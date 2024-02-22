#include <stdio.h>
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

int main(int argc, char **argv){

    struct server_info SERVER;
    pthread_t send_t, recv_t;

    SERVER.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    SERVER.addr.sin_port = htons(SERVER_PORT);
    SERVER.addr.sin_family = AF_INET;
    SERVER.addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    SERVER.addr_len = sizeof(SERVER.addr);

    while(1){
        if(connect(SERVER.sockfd, (struct sockaddr*)&SERVER.addr, SERVER.addr_len) == 0){
            break;
        }
        printf("trying again\n");
        sleep(1);
    }
    getchar();
    send(SERVER.sockfd, "hi", 2, 0);
}
