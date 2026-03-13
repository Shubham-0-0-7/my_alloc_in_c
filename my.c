#include <stdio.h>
#include <unistd.h>

struct header{
  size_t s;
  header* next;
}

int main(void){
    printf("hellow\n");
    void* start = sbrk(0);
    void* new_mem = sbrk(1024);
    
}
