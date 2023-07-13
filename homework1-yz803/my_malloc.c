#include "my_malloc.h"


// These three varibles are global variable.
unsigned long data_segment_size = 0;    // a variable available storage
unsigned long free_space_size = 0;      // the sum of the free space 
unsigned long largest_free_block = 0;   // the largest block of the free space

// These are three global pointers
free_p *first_free = NULL;            //the address of the first free block
  free_p *last_free = NULL;           //the address of the last free block
  free_p *start_place = NULL;         //the address of the initial address

/*
This function takes a size to malloc and return the address of this space.
*/
void * ff_malloc(size_t size) {
    if(first_free == NULL) {          //there's no allocated space so using call_new_space to get one
        return call_new_space(size);
    }
    else {                            //there is allocated space
        free_p *cur = first_free;
        while(cur != NULL) {          //find a space that can allocate the space requesteds
            if(cur->size >= size) {       //using the double linklist to traverse the free areas
                return reuse_space(size, cur);
            }
            cur = cur->next;
        }
        return call_new_space(size);
    }
}

//this function using system call to get a new space in the heap 
void * call_new_space(size_t size) {
    free_p *new_space = sbrk(size + sizeof(free_p));      // the struct also occupies space
    if((void *)new_space == (void *) -1) {                //check wheteher the space is allocate for us
        return NULL;
    }
    data_segment_size += size + sizeof(free_p);       
    if(start_place == NULL) {                             //get the initial address
        start_place = new_space;
    }
    init_free_space(new_space, size);                     //initialize the free space
    new_space->is_free = 'n';                             //n is short for no means that the space is not free
    return (char *)new_space + sizeof(free_p);
}

// This function reuse the space being freed, the cur is the struct being operated
void * reuse_space(size_t size, free_p *cur) {
    delete_used_space(cur);                               //the space will be used and is not free
    if(cur->size > (size + sizeof(free_p))) {   //if the extra space is less than sizeof(free_p), then the space cannot use
        free_p *new_space = (free_p *) ((char *)cur + sizeof(free_p) + size);
        init_free_space(new_space, cur->size - size - sizeof(free_p));    // if there are enough space then a new free area was born
        cur->size = size; 
        add_free_space(new_space);
        free_space_size -= (size + sizeof(free_p));
    }
    else {
        free_space_size -= (cur->size + sizeof(free_p));
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
void delete_used_space(free_p * p) {
  if ((last_free == first_free) && (last_free == p)) {    //if the linklist is empty
    last_free = first_free = NULL;
  }
  else if (last_free == p) {              //if the node is at the last
    last_free = p->prev;
    last_free->next = NULL;
  }
  else if (first_free == p) {             //if the node is at the first
    first_free = p->next;
    first_free->prev = NULL;
  }
  else {
    p->prev->next = p->next;
    p->next->prev = p->prev;
  }
}

/*
This function is used for adding a new node in the linklist
*/
void add_free_space(free_p * p) {
  if ((first_free == NULL) || (p < first_free)) {     //if the node is ahead of the first_free
    p->prev = NULL;
    p->next = first_free;
    if (p->next != NULL) {                           //if the node has a latter node
      p->next->prev = p;
    }
    else {
      last_free = p;
    }
    first_free = p;
  }
  else {
    free_p * curr = first_free;
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
      last_free = p;
    }
  }
}

/*
free function is the pair of the ff_malloc function, used to free a block
*/
void ff_free(void * ptr) {
    if(ptr == NULL) {         //if the address is not in the heap here then do nothing and return
        return ;
    }
    free_p *comb = (free_p*)((char *)ptr - sizeof(free_p));   //find the address corresponding
    if(comb->is_free != 'n') {                                //if the area has been freed or the address is not valid then exit
      exit(0);
    }
    comb->is_free = 'y';
    free_space_size += (comb->size + sizeof(free_p));
    add_free_space(comb);                                       //add a node in the free linklist
    if(comb->next != NULL && (char*)(comb) + comb->size + sizeof(free_p) == (char *)comb->next)  {    //combine the block after
        comb->size += (comb->next->size + sizeof(free_p));
        delete_used_space(comb->next);
    }
    if(comb->prev != NULL && (char*)(comb->prev) + comb->prev->size + sizeof(free_p) == (char *)comb) { //combine the block before
        comb->prev->size += (comb->size + sizeof(free_p));
        delete_used_space(comb);
    }
}

/*
The function using best fit strategy and allocate memeory of size size
*/
void * bf_malloc(size_t size) {
    free_p *temp = first_free, *min = NULL;
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
        return call_new_space(size);
    }
    else {
        return reuse_space(size, min);
    }
}

/*
The function is used for free an area being allocated and here the function is the same as the ff_free
*/
void bf_free(void * ptr) {
    ff_free(ptr);
}

/*
The test required function, return with the value of the largest_free_block
*/
unsigned long get_largest_free_data_segment_size() {
    if(start_place == NULL) {
      return 0;
    }
    else {
      free_p *temp = start_place;
      while(temp != NULL) {                 //traverse the linklist and find the max one
        if(temp->size > largest_free_block) {
          largest_free_block = temp->size;
        }
        temp = temp->next;
      }
      return largest_free_block; 
    }
}

/*
The test required function, return with the value of the free_space_size
*/
unsigned long get_total_free_size() {
    return free_space_size;
}