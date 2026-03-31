#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "slab.h"

slab_t* slab_create(size_t obj_size){
  void* page = mmap(
      NULL,
      PAGE_SIZE,
      PROT_READ | PROT_WRITE,
      MAP_ANONYMOUS | MAP_PRIVATE,
      -1,0);
  if(page == MAP_FAILED) return NULL;
  slab_t* slab = (slab_t*)page;

  if(obj_size < sizeof(slot_t)) obj_size = sizeof(slot_t);
  
  slab->obj_size = obj_size;
  slab->next = NULL;

  char* slot_start = (char*)(slab+1);
  size_t available = PAGE_SIZE-sizeof(slab_t);
  slab->num_free = available/obj_size;

  if(slab->num_free==0){
    munmap(page, PAGE_SIZE);
    return NULL;
  }

  slab->free_list = (slot_t*)slot_start;
  slot_t* curr_slot=slab->free_list;

  for(size_t i=0; i<slab->num_free-1; i++){
    slot_t* next_slot = (slot_t*)((char*)curr_slot+obj_size);
    curr_slot->next = next_slot;
    curr_slot = next_slot;
  }
  curr_slot->next=NULL;
  return slab;
}

void* slab_alloc(slab_t* slab){
  if(slab==NULL || slab->free_list==NULL) return NULL;
  slot_t* aslot = slab->free_list;
  slab->free_list = aslot->next;
  slab->num_free--;
  return (void*)aslot;
}

void slab_free(slab_t* slab, void* ptr){
  if(slab==NULL || ptr== NULL) return;
  slot_t* freed_slot = (slot_t*)ptr;
  freed_slot->next=slab->free_list;
  slab->free_list = freed_slot;
  slab->num_free++;
}

pool_t* pool_create(size_t obj_size){
  pool_t* pool = mmap(
      NULL,
      sizeof(pool_t),
      PROT_READ | PROT_WRITE, 
      MAP_ANONYMOUS | MAP_PRIVATE,
      -1, 0);

  if(pool==MAP_FAILED) return NULL;
  pool->obj_size=obj_size;
  pool->partial = NULL;
  return pool;
}

void* pool_alloc(pool_t* pool){
  if(pool==NULL) return NULL;
  slab_t* curr = pool->partial;
  while(curr!=NULL){
    if(curr->num_free > 0) return slab_alloc(curr);
    curr=curr->next;
  }
  
  slab_t* new_slab = slab_create(pool->obj_size);
  if(new_slab==NULL) return NULL;
  new_slab->next = pool->partial;
  pool->partial=new_slab;

  return slab_alloc(new_slab);
}

void pool_free(pool_t* pool, void* ptr){
  if(pool==NULL || ptr==NULL) return;
  uintptr_t page_start = (uintptr_t)ptr & ~((uintptr_t)PAGE_SIZE-1);
  slab_t* slab = (slab_t*)page_start;
  slab_free(slab, ptr);
}


int main(){
  slab_t* my_slab = slab_create(64);
  if(my_slab != NULL) printf("created a slab with %zu free slots.\n", my_slab->num_free);
  else printf("slab creation failed\n");
  return 0;
}
