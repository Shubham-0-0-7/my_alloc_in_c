#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <pthread.h>
#include <stdalign.h>
#include <string.h>
#include <stdint.h>

#define MIN_SPLIT_SIZE 16

typedef struct header{
  size_t size;
  unsigned is_free;
  struct header* next;
  struct header* prev;
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

  if(header->next && header->next->is_free){
    char* block_a_end = (char*)header+sizeof(header)+header->size;
    if(block_a_end == (char*)header->next){
      header_t* block_b = header->next; 
      header->size += sizeof(header_t)+block_b->size;
      header->next = block_b->next;

      if(header->next) header->next->prev = header;

      if(tail== block_b) tail = header;
    }
  }
  if(header->prev && header->prev->is_free){
    header_t* prev_block = header->prev;
    char* prev_block_end = (char*)prev_block + sizeof(header_t)+prev_block->size;
    if(prev_block_end == (char*)header){
      prev_block->size += sizeof(header_t)+header->size;
      prev_block->next = header->next;

      if(prev_block->next) prev_block->next->prev = prev_block;
      if(tail==header) tail = prev_block;
    }
  }
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
    if(header->size >= size + sizeof(header_t)+MIN_SPLIT_SIZE){
      header_t* new_header = (header_t*)((char*)(header+1)+size);
      new_header->size = header->size - size - sizeof(header_t);
      new_header->is_free = 1;
      new_header->next = header->next;
      new_header->prev = header;

      if(new_header->next) new_header->next->prev = new_header;

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
  header->prev = tail;

  if(!head) head = header;
  if(tail) tail->next = header;

  tail = header;
  pthread_mutex_unlock(&global_malloc_lock);
  return (void *)(header+1);

}

void* my_realloc(void* ptr, size_t new_size){
  if(!ptr) return my_alloc(new_size);
  if(new_size == 0){
    my_free(ptr);
    return NULL;
  }
  header_t* header = (header_t*)ptr-1;
  if(header->size >= new_size) return ptr; //little optimization 

  void* new_ptr = my_alloc(new_size);
  if(new_ptr ==NULL) return NULL;
  memcpy(new_ptr, ptr, header->size);
  my_free(ptr);
  return new_ptr;
}

void* my_calloc(size_t num, size_t size){
  if(num==0 || size==0) return NULL;
  if(num > SIZE_MAX/size) return NULL;
  //because if we did num*size > size_max then it will overflow before evaluating
  size_t totalsize = num*size;
  void* ptr = my_alloc(totalsize);
  if(ptr) memset(ptr, 0, totalsize);
  return ptr;
}

void printmemlist(){
  header_t* curr = head;
    printf("current memory state\n");
    int block_num =1;
    while(curr){
        printf("block %d: addr = %p \nsize = %zu bytes \nis_free = %d \nnext = %p\n", 
            block_num, 
            (void*)curr, 
            curr->size, 
            curr->is_free, 
            (void*)curr->next
        );
        curr = curr->next;
        block_num++;
  }
  printf("\t\n");
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




