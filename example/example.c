#include "../scm.h"

void* client_handle(int fd, void* x){
    int *i = (int*)x;
    char buf[2];
    recv(fd, buf, 2, 0);
    printf("%.2s received with arg %d on fd %d\n", buf, *i, fd);
    return i;
}

int main(){
    int y = 5;
    server_args s1;
    s1.fptr = client_handle;
    s1.args = &y;
    create_server(&s1, 100000, 4, STATEFUL);
}
