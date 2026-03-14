#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <pthread.h>
#include <stdalign.h>

typedef struct header{
  size_t size;
  unsigned is_free;
  struct header* next;
} __attribute__((aligned(16))) header_t;

header_t* head = NULL;
header_t* tail = NULL;
pthread_mutex_t global_malloc_lock = PTHREAD_MUTEX_INITIALIZER;

header_t* get_free_block(size_t s){
  header_t* curr = head;
  while(curr){
    if(curr->is_free && curr->size >= s) return curr;
    curr = curr->next;
  }
  return NULL;
}

void* my_alloc(size_t size){
  /*void *block
  block = sbrk(size);
  if(block == (void*)-1) return nullptr;
  return block;
  */ 
  
  size_t totalsize;
  void* block;
  header_t* header;
  if(!size) return NULL;
  pthread_mutex_lock(&global_malloc_lock);

  header = get_free_block(size);
  if(header){
    header->is_free = 0;
    pthread_mutex_unlock(&global_malloc_lock);
    return (void *)(header+1);
  }
  totalsize = sizeof(header_t)+size;
  block = sbrk(totalsize);
  if(block==(void *)-1){
    pthread_mutex_unlock(&global_malloc_lock);
    return NULL;
  }

  header = (header_t*)block;
  header->size = size;
  header->is_free = 0;
  header->next = NULL;

  if(!head) head = header;
  if(tail) tail->next = header;

  tail = header;
  pthread_mutex_unlock(&global_malloc_lock);
  return (void *)(header+1);

}


int main(void){
    int *a = my_alloc(sizeof(int));
    int *b = my_alloc(sizeof(int));

    *a = 10;
    *b = 20;

    printf("a = %d\n", *a);
    printf("b = %d\n", *b);

    printf("a address = %p\n", a);
    printf("b address = %p\n", b);

    return 0;
}




