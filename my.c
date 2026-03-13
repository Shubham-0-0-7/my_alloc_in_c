#include <stdio.h>
#include <unistd.h>

struct header{
  size_t size;
  unsigned is_free;

}
void my_alloc(size_t size){
  void *block
  block = sbrk(size);
  if(block == (void*)-1) return NULL;
  return block;
}


int main(void){

}




