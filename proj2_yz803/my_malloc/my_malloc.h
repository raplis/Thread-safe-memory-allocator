#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


//This struct contains the information of the alloc space
struct free_ptr {
    size_t size;        //the address of the block
    struct free_ptr *prev;      //the block before
    struct free_ptr *next;      //the block after
    char is_free;       //since the answer is yes or no, using char instead of int can reduce space
};

typedef struct free_ptr free_p;

void * ts_malloc_lock(size_t size);
void ts_free_lock(void * ptr);
void * ts_malloc_nolock(size_t size);
void ts_free_nolock(void * ptr);
void * bf_malloc(size_t size, free_p **first_free, free_p ** last_free,int sbrk_lock);
void bf_free(void * ptr, free_p **first_free, free_p ** last_free);

void * call_new_space(size_t size, int sbrk_lock);
void * reuse_space(size_t size, free_p *cur, free_p **first_free, free_p ** last_free);
void init_free_space(free_p *ptr, size_t size);
void delete_used_space(free_p *p, free_p **first_free, free_p ** last_free);
void add_free_space(free_p *ptr, free_p **first_free, free_p ** last_free);
