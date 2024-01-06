#include "../scm.h"

void* client_handle(void* x){
    int *i = (int*)x;
    printf("received value %d\n", *i);
    return i;
}

int main(){
    int y = 5;
    server_args s1;
    s1.fptr = client_handle;
    s1.args = &y;
    create_server(&s1, 100010, 1000);
}
