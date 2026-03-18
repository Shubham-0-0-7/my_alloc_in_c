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

void my_free(void* ptr){
  header_t* header;
  if(!ptr) return;
  pthread_mutex_lock(&global_malloc_lock);
  header = (header_t*)ptr - 1;
  header->is_free = 1;
  pthread_mutex_unlock(&global_malloc_lock);
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
    if(header->size >= size + sizeof(header_t)){
      header_t* new_header = (header_t*)((char*)(header+1)+size);
      new_header->size = header->size - size - sizeof(header_t);
      new_header->is_free = 1;
      new_header->next = header->next;

      header->size = size;
      header->next = new_header;
      if(header==tail) tail = new_header;
    }
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

void printmemlist(){
  header_t* curr = head;
    printf("current memory state\n");
    int block_num =1;
    while(curr){
        printf("block %d: addr = %p \n size = %zu bytes \n is_free = %d \n next = %p\n", 
            block_num, 
            (void*)curr, 
            curr->size, 
            curr->is_free, 
            (void*)curr->next
        );
        curr = curr->next;
        block_num++;
  }
  printf("cshdabcjhdjdvchj\n");
}

int main(void){
    printf("block of size 100 alloc\n");
    void *ptr1 = my_alloc(100);
    printmemlist();
    printf("freeing block of size 100\n");
    my_free(ptr1);
    printmemlist();

    printf("smaller block allocate 20\n");
    void *ptr2 = my_alloc(20);
    printmemlist();
/*
    int *a = my_alloc(sizeof(int));
    int *b = my_alloc(sizeof(int));
    *a = 10;
    *b = 20;
    printf("a = %d\n", *a);
    printf("b = %d\n", *b);
    printf("a address = %p\n", a);
    printf("b address = %p\n", b);
*/
    return 0;
}




