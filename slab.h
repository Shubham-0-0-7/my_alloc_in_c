#ifndef SLAB_H
#define SLAB_H

#define PAGE_SIZE 4096

typedef struct slot{
  struct slot* next;
}slot_t;

typedef struct slab{
  slot_t* free_list;
  size_t obj_size;
  size_t num_free;
  struct slab* next;
}slab_t;

typedef struct pool{
  slab_t* partial;
  size_t obj_size;
}pool_t;

slab_t* slab_create(size_t obj_size);
void* slab_alloc(slab_t* slab);
void slab_free(slab_t* slab, void* ptr);

pool_t* pool_create(size_t obj_size);
void* pool_alloc(pool_t* pool);
void pool_free(pool_t* pool, void* ptr);
 
#endif
