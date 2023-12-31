#include "scm.h"

int NUM_CLIENTS = 0;
int MAX_CLIENTS = 1000;
int MAX_THREADS = 10000;
int epoll_fd;
threadqueue *QUEUE;
client_info **CLIENTS;
pthread_mutex_t mutex;
pthread_cond_t cond;

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

void* thread_f(server_args *handler){

    while(1){

        NODE *temp = NULL;
        pthread_mutex_lock(&mutex);
        while(QUEUE->num_nodes == 0){
            pthread_cond_wait(&cond, &mutex);
        }
        temp = dequeue();
        pthread_mutex_unlock(&mutex);

        handler->fptr(handler->args);

        struct epoll_event event;
        event.data.fd = temp->fd;
        event.events = EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;

        if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, temp->fd, &event) == -1){
            printf("epoll_ctl client");
        }
    }
    return 0;
}

void handshake(client_info *client){
    char buf[8];
    snprintf(buf, 8, "%d", client->id);
    send(client->sockfd ,buf, 8, 0);
}


int create_server(server_args *handler, int MAX_CLIENTS, int MAX_THREADS, int HANDSHAKE){
    
    int s_socket;
    int flags;
    int reuse;
    socklen_t s_addr_len;
    struct sockaddr_in s_addr;
    struct epoll_event event;
    struct epoll_event event_arr[MAX_CLIENTS +1];
    pthread_t t_pool[MAX_THREADS];

    s_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(s_socket == -1){
        printf("error creating socket\n");
        return -1;
    }

    flags = fcntl(s_socket, F_GETFL, 0);
    fcntl(s_socket, F_SETFL, flags | O_NONBLOCK);

    reuse = 1;
    if(setsockopt(s_socket, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse))<0){
        printf("setting SO_REUSEPORT failed\n");
        exit(-1);
    }

    s_addr.sin_port = htons(SERVER_PORT);
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = INADDR_ANY;
    s_addr_len = sizeof(s_addr);

    epoll_fd = epoll_create1(0);
    if(epoll_fd == -1){
        printf("epoll_create1");
        return -1;
    }

    event.events = EPOLLIN;
    event.data.fd = s_socket;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, s_socket, &event) == -1){
        printf("epoll_ctl");
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


    CLIENTS = (client_info**)malloc(MAX_CLIENTS*sizeof(client_info*));
    QUEUE = (threadqueue*)malloc(sizeof(threadqueue));

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    for(int i=0; i<MAX_THREADS; i++){
        if(pthread_create(&t_pool[i], NULL, (void*)&thread_f, (void*) handler) != 0){
            printf("error creating thread:%d\n", i+1);
        }
    }

    while(1){

        int num_events = epoll_wait(epoll_fd, event_arr, MAX_CLIENTS, -1);
        if(num_events == -1){
            printf("epoll wait error");
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
                    event.events = EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;

                    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_client->sockfd, &event) == -1){
                        printf("epoll_ctl client");
                        close(new_client->sockfd);
                    }

                    CLIENTS[NUM_CLIENTS] = new_client;
                    NUM_CLIENTS++;

                    if(HANDSHAKE == 1){
                        handshake(new_client);
                    }
                }
                else{
                    free(new_client);
                }
            }
            else if(event_arr[i].events & EPOLLRDHUP){
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_arr[i].data.fd, NULL);
                close(event_arr[i].data.fd);
            }
            else if(event_arr[i].events & EPOLLIN){
                int fd = event_arr[i].data.fd;
                pthread_mutex_lock(&mutex);
                enqueue(fd);
                pthread_mutex_unlock(&mutex);
                pthread_cond_signal(&cond);
            }
        }
    }
}
