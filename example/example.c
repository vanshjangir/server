#include "../scm.h"

void* client_handle(void* x){
    int *i = (int*)x;
    printf("value is %d\n", *i);
    return i;
}

int main(){
    int y = 7;
    server_args s1;
    s1.fptr = client_handle;
    s1.args = &y;
    create_server(&s1, 1000, 1000, 0);
}
