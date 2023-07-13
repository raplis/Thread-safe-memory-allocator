#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


//This struct contains the information of the alloc space
struct free_ptr {
    size_t size;        //the address of the block
    struct free_ptr *prev;      //the block before
    struct free_ptr *next;      //the block after
    char is_free;       //since the answer is yes or no, using char instead of int can reduce space
};

typedef struct free_ptr free_p;

void * ff_malloc(size_t size);
void ff_free(void * ptr);
void * bf_malloc(size_t size);
void bf_free(void * ptr);

void * call_new_space(size_t size);
void * reuse_space(size_t size, free_p *cur);
void init_free_space(free_p *ptr, size_t size);
void delete_used_space(free_p *p);
void add_free_space(free_p *ptr);

unsigned long get_largest_free_data_segment_size();
unsigned long get_total_free_size();