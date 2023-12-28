#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define MAX_THREADS 10
#define INITIAL_ID 10000000

typedef struct{
    int sockfd;
    int id;
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
    char type[4];
    int receiver_id;
    int sender_id;
    char *message;
    int msg_size;
} request;

int NUM_CLIENTS = 0;
int MAX_CLIENTS = 1000;
threadqueue *QUEUE;
client_info **CLIENTS;
pthread_mutex_t mutex;
pthread_cond_t cond;

NODE* get_node(int);
void enqueue(int);
NODE* dequeue();
int receive_buf(int,request*);
void send_buf(request*);
void handle_client(int);
request* parse(char[],int);
void fetch(int,char[]);
void* thread_f();
void handshake(client_info*);
int get_fd(int);

NODE* get_node(int val){
    NODE *temp = (NODE*)malloc(sizeof(NODE));
    temp->fd = val;
    temp->link = NULL;
    return temp;
}

void enqueue(int val){

    NODE *temp = get_node(val);
    if(QUEUE->num_nodes == 0){
        QUEUE->head = temp;
        QUEUE->rear = temp;
    }else{
        QUEUE->rear->link = temp;
        QUEUE->rear = temp;
    }

    QUEUE->num_nodes++;
}

NODE* dequeue(){
    if(QUEUE->num_nodes == 0){
        return NULL;
    }
    else{
        NODE *temp = QUEUE->head;
        QUEUE->head = QUEUE->head->link;
        QUEUE->num_nodes--;

        if(QUEUE->num_nodes == 0){
            QUEUE->head = NULL;
            QUEUE->rear = NULL;
        }
        
        return temp;
    }
}

int receive_buf(int fd, request *req){

    char header[9];
    if(recv(fd, header, 8, 0) <= 0){
        return -1;
    }
    
    header[8] = '\0';

    char id[9];
    int len = atoi(header);
    char buffer[len +1];

    recv(fd, buffer, len, 0);
    
    for(int i=0; i<3; i++)
        req->type[i] = buffer[i];

    for(int i=0; i<8; i++)
        id[i] = buffer[4+i];
    id[8] = '\0';
    req->sender_id = atoi(id);

    for(int i=0; i<8; i++)
        id[i] = buffer[13+i];
    id[8] = '\0';
    req->receiver_id = atoi(id);

    req->message = (char*)malloc((len-22)*sizeof(char));
    for(int i=0; i<len-23; i++)
        req->message[i] = buffer[22+i];
    req->message[len-23] = '\0';
    req->msg_size = len-23;

    return 0;
}

int get_fd(int r_id){
    for(int i=0; i<NUM_CLIENTS; i++){
        if(CLIENTS[i]->id == r_id)
            return CLIENTS[i]->sockfd;
    }
    return -1;
}

void send_buf(request *req){
    
    char header[8];
    int size = req->msg_size;
    int fd = get_fd(req->receiver_id);
    for(int i=7; i>=0; i--){
        header[i] = (size%10)+'0';
        size /= 10;
    }
    send(fd, header, 8, 0);
    send(fd, req->message, req->msg_size, 0);
}

void handle_client(int fd){
    request *req;
    req = (request*)malloc(sizeof(request));
    
    if(receive_buf(fd, req) != 0)
        return;
    if(strncmp(req->type, "FCH", 3) == 0){
        //fetch fn
    }
    else if(strncmp(req->type, "MSG", 3) == 0){
        send_buf(req);
    }
}

void* thread_f(){

    while(1){

        NODE *temp = NULL;
        pthread_mutex_lock(&mutex);
        while(QUEUE->num_nodes == 0){
            pthread_cond_wait(&cond, &mutex);
        }
        temp = dequeue();
        pthread_mutex_unlock(&mutex);

        handle_client(temp->fd);
    }
    return 0;
}

void handshake(client_info *client){
    char buf[8];
    snprintf(buf, 8, "%d", client->id);
    send(client->sockfd ,buf, 8, 0);
}


int main(int argc, char *argv[]){
    
    if(argc > 1){
        MAX_CLIENTS = atoi(argv[1]);
    }

    int s_socket;
    int epoll_fd;
    int flags;
    socklen_t s_addr_len;
    struct sockaddr_in s_addr;
    struct epoll_event event;
    struct epoll_event event_arr[MAX_CLIENTS +1];
    pthread_t t_pool[MAX_THREADS];

    s_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(s_socket == -1){
        perror("socket");
        return -1;
    }

    flags = fcntl(s_socket, F_GETFL, 0);
    fcntl(s_socket, F_SETFL, flags | O_NONBLOCK);

    s_addr.sin_port = htons(SERVER_PORT);
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = INADDR_ANY;
    s_addr_len = sizeof(s_addr);

    epoll_fd = epoll_create1(0);
    if(epoll_fd == -1){
        perror("epoll_create1");
        return -1;
    }

    event.events = EPOLLET | EPOLLIN;
    event.data.fd = s_socket;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, s_socket, &event) == -1){
        perror("epoll_ctl");
        return -1;
    }


    if(bind(s_socket, (struct sockaddr*)&s_addr, s_addr_len) != 0){
        printf("bind failed");
        close(s_socket);
        return -1;
    }

    if(listen(s_socket, MAX_CLIENTS) != 0){
        printf("listen failed");
        close(s_socket);
        return -1;
    }

    printf("listening on port 8080\n");

    CLIENTS = (client_info**)malloc(MAX_CLIENTS*sizeof(client_info*));
    QUEUE = (threadqueue*)malloc(sizeof(threadqueue));

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    for(int i=0; i<MAX_THREADS; i++){
        if(pthread_create(&t_pool[i], NULL, (void*)&thread_f, NULL) != 0){
            printf("error creating thread:%d\n", i+1);
        }
    }

    while(1){

        int num_events = epoll_wait(epoll_fd, event_arr, MAX_CLIENTS, -1);
        if(num_events == -1){
            perror("epoll");
            return -1;
        }

        for(int i=0; i<num_events; i++){
            if(event_arr[i].data.fd == s_socket){
                client_info *new_client;
                new_client = (client_info*)malloc(sizeof(client_info));
                new_client->addr_len = sizeof(new_client->addr);
                
                new_client->sockfd = accept(s_socket,
                        (struct sockaddr*)&new_client->addr,
                        &new_client->addr_len);

                if(new_client->sockfd != -1){
                    int c_flags = fcntl(new_client->sockfd, F_GETFL, 0);
                    fcntl(new_client->sockfd, F_SETFL, c_flags | O_NONBLOCK);
                    new_client->id = INITIAL_ID+NUM_CLIENTS+1;

                    event.data.fd = new_client->sockfd;
                    event.events = EPOLLIN | EPOLLET;

                    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_client->sockfd, &event) == -1){
                        printf("epoll_ctl client");
                        close(new_client->sockfd);
                    }else{
                        printf("new connections %d\n", NUM_CLIENTS+1);
                    }

                    CLIENTS[NUM_CLIENTS] = new_client;
                    NUM_CLIENTS++;
                }
                else{
                    free(new_client);
                }
            }
            else{
                int fd = event_arr[i].data.fd;
                pthread_mutex_lock(&mutex);
                enqueue(fd);
                pthread_mutex_unlock(&mutex);
                pthread_cond_signal(&cond);
            }
        }
    }
}
