#include <stdio.h>
#include <unistd.h>

int main(void){
    char *mem = (char*)sbrk(0);
    mem = (char*)sbrk(10);
    *mem = "helloneell";

}
