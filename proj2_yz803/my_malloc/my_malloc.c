#include "my_malloc.h"



// These are three global pointers
free_p *first_free = NULL;            //the address of the first free block
free_p *last_free = NULL;           //the address of the last free block

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
__thread free_p *first_free_nolock = NULL;
__thread free_p *last_free_nolock = NULL;

void * ts_malloc_lock(size_t size) {
  pthread_mutex_lock(&lock);
  int sbrk_lock = 0;
  void * p = bf_malloc(size, &first_free, &last_free, sbrk_lock);
  pthread_mutex_unlock(&lock);
  return p;
}
void ts_free_lock(void * ptr) {
  pthread_mutex_lock(&lock);
  bf_free(ptr, &first_free, &last_free);
  pthread_mutex_unlock(&lock);
}

void * ts_malloc_nolock(size_t size) {
  int sbrk_lock = 1;
  void * p = bf_malloc(size, &first_free_nolock, &last_free_nolock, sbrk_lock);
  return p;
}
void ts_free_nolock(void * ptr) {
  bf_free(ptr, &first_free_nolock, &last_free_nolock);
}

//this function using system call to get a new space in the heap 
void * call_new_space(size_t size, int sbrk_lock) {
    free_p *new_space = NULL;
    if (sbrk_lock == 0) {
      new_space = sbrk(size + sizeof(free_p));
    }
    else {
      pthread_mutex_lock(&lock);
      new_space = sbrk(size + sizeof(free_p));
      pthread_mutex_unlock(&lock);
    }
    if((void *)new_space == (void *) -1) {                //check wheteher the space is allocate for us
        return NULL;
    }
    // if(start_place == NULL) {                             //get the initial address
    //     start_place = new_space;
    // }
    init_free_space(new_space, size);                     //initialize the free space
    new_space->is_free = 'n';                             //n is short for no means that the space is not free
    return (char *)new_space + sizeof(free_p);
}

// This function reuse the space being freed, the cur is the struct being operated
void * reuse_space(size_t size, free_p *cur, free_p **first_free, free_p **last_free) {
    delete_used_space(cur, first_free, last_free);                               //the space will be used and is not free
    if(cur->size > (size + sizeof(free_p))) {   //if the extra space is less than sizeof(free_p), then the space cannot use
        free_p *new_space = (free_p *) ((char *)cur + sizeof(free_p) + size);
        init_free_space(new_space, cur->size - size - sizeof(free_p));    // if there are enough space then a new free area was born
        cur->size = size; 
        add_free_space(new_space, first_free, last_free);
    }
    else {
    }
    cur->is_free = 'n';                                           //the area is used and not free anymore    
    return (char *)cur + sizeof(free_p);
}

//initialize the block diameter
void init_free_space(free_p *ptr, size_t size) {
        ptr->is_free = 'y';
        ptr->size = size;
        ptr->prev = NULL;
        ptr->next = NULL;
}

/*
This function is used for deleting a block which was free and not free anymore.
You can consider it as delete a node in the double linklist.
*/
void delete_used_space(free_p * p, free_p **first_free, free_p **last_free) {
  if ((*last_free == *first_free) && (*last_free == p)) {    //if the linklist is empty
    *last_free = *first_free = NULL;
  }
  else if (*last_free == p) {              //if the node is at the last
    *last_free = p->prev;
    (*last_free)->next = NULL;
  }
  else if (*first_free == p) {             //if the node is at the first
    *first_free = p->next;
    (*first_free)->prev = NULL;
  }
  else {
    p->prev->next = p->next;
    p->next->prev = p->prev;
  }
}

/*
This function is used for adding a new node in the linklist
*/
void add_free_space(free_p * p, free_p **first_free, free_p **last_free) {
  if ((*first_free == NULL) || (p < *first_free)) {     //if the node is ahead of the first_free
    p->prev = NULL;
    p->next = *first_free;
    if (p->next != NULL) {                           //if the node has a latter node
      p->next->prev = p;
    }
    else {
      *last_free = p;
    }
    *first_free = p;
  }
  else {
    free_p * curr = *first_free;
    while ((curr->next != NULL) && (p > curr->next)) {      //if in the middle or at last then traverse the linklist to find the exact place
      curr = curr->next;  
    }
    p->prev = curr;
    p->next = curr->next;
    curr->next = p;
    if (p->next != NULL) {
      p->next->prev = p;
    }
    else {
      *last_free = p;
    }
  }
}



/*
The function using best fit strategy and allocate memeory of size size
*/
void * bf_malloc(size_t size, free_p **first_free, free_p **last_free, int sbrk_lock) {
    free_p *temp = *first_free, *min = NULL;
    while(temp != NULL) {               //traverse the linklist to find the best fit space  
        if(temp->size > size) {    
            if(min == NULL) {
                min = temp;
            }
            else if(min->size > temp->size) {
                min = temp;
            }
        }
        if (temp->size == size) {       //a suger to reduce time if an area is fit
          min = temp;
          break;
        }
        temp = temp->next;
    }
    if(min == NULL) {                 //if there's no suitable space then using sbrk() to get new space
        return call_new_space(size, sbrk_lock);
    }
    else {
        return reuse_space(size, min, first_free, last_free);
    }
}

/*
The function is used for free an area being allocated and here the function is the same as the ff_free
*/
void bf_free(void * ptr, free_p **first_free, free_p **last_free) {
    if(ptr == NULL) {         //if the address is not in the heap here then do nothing and return
        return ;
    }
    free_p *comb = (free_p*)((char *)ptr - sizeof(free_p));   //find the address corresponding
    if(comb->is_free != 'n') {                                //if the area has been freed or the address is not valid then exit
      exit(0);
    }
    comb->is_free = 'y';
    add_free_space(comb, first_free, last_free);                                       //add a node in the free linklist
    if(comb->next != NULL && (char*)(comb) + comb->size + sizeof(free_p) == (char *)comb->next)  {    //combine the block after
        comb->size += (comb->next->size + sizeof(free_p));
        delete_used_space(comb->next, first_free, last_free);
    }
    if(comb->prev != NULL && (char*)(comb->prev) + comb->prev->size + sizeof(free_p) == (char *)comb) { //combine the block before
        comb->prev->size += (comb->size + sizeof(free_p));
        delete_used_space(comb, first_free, last_free);
    }
}

