#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "mymemory.h"

//#define NDEBUG
#ifndef NDEBUG
#define debug(x) x
#else
#define debug(x) ((void)0)
#endif
#ifndef NDEBUG
void check_linklist_impl(node *head) {
  while(head)
    head = head->next;
}

void assert_impl(
  int truth,
  const char* filename,
  int lineno,
  const char* func
) {
  if(!truth) {
    fprintf(stderr, "at %s:%d function:%s", filename, lineno, func);
    abort();
  }
}
#endif

#define check_linklist(x) debug(check_linklist_impl(x))
#define myassert(x) debug(assert_impl((x), __FILE__, __LINE__, __func__))

/* mymalloc_init: initialize any data structures that your malloc needs in
                  order to keep track of allocated and free blocks of 
                  memory.  Get an initial chunk of memory for the heap from
                  the OS using sbrk() and mark it as free so that it can  be 
                  used in future calls to mymalloc()
*/
node *freeHead=NULL;
void *begin=NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


int mymalloc_init() {
	// placeholder so that the program will compile
  // lock
  pthread_mutex_lock(&lock);
  void *begin = sbrk(32*page);
  if(begin!=(void *) -1){
    node *head = begin;
    head->size = page - sizeof(node);
    head->next = NULL;
    freeHead = head;
    // unlock
    pthread_mutex_unlock(&lock);
    return 0;
  }
  // unlock
  pthread_mutex_unlock(&lock);
	return 1; // non-zero return value indicates an error
}


/* mymalloc: allocates memory on the heap of the requested size. The block
             of memory returned should always be padded so that it begins
             and ends on a word boundary.
     unsigned int size: the number of bytes to allocate.
     retval: a pointer to the block of memory allocated or NULL if the 
             memory could not be allocated. 
             (NOTE: the system also sets errno, but we are not the system, 
                    so you are not required to do so.)
*/
/*int findSize(unsigned int requestSize){
  int i = 1;
  while(requestSize> i*8){
    i++;
  }
  i++;
  return i*8;
}
*/
 
void *mymalloc(unsigned int size) {
  // lock the crital session
  
  pthread_mutex_lock(&lock);

  int totalSize=0;
  int bollean=0;
  int mod = size%8;
  int headSize = sizeof(header);
  
  if(size<8)size=8;//FIXME
  
  if(mod!=0){
    totalSize = size+headSize+8-mod;
  }else{
    totalSize = size + headSize;
  }
  totalSize = max(totalSize, sizeof(header), sizeof(freeListNode))
  
  node *temp = freeHead;
  node *parent = freeHead;
    //find the first sufficient memory blockp/
  while(temp!=NULL && temp->size < totalSize){
    parent=temp;
    temp = temp->next;
  }
  if(temp==NULL){
    // calculate the size we need to increase
    int increaseSize = size + 4*page - size%page;
    if(mymalloc_increase(increaseSize)==0){
      parent->size += increaseSize;
      // memcopy the freehead node struct to the new place
      node *const src = parent;
      int newFreeSize = parent->size - totalSize;
      parent->size = newFreeSize;
      node *dest = (node *)((char *)parent + totalSize);
      memmove(dest, src, sizeof(node));
  //input the meta data for the allocation block
      header *allocatedHead = (header *)parent;
      allocatedHead->size = totalSize-headSize;
      allocatedHead->magicNumber = MN;
      void *retPtr = (void *)((char *)allocatedHead + headSize);
      // seach the grandparent node
      if(freeHead->next!=NULL){
        node *grandparent=freeHead;
        while((char *)grandparent->next!= (char *)parent){
          grandparent= grandparent->next;
        }
          // allocate the require memory
        // copy the node struct below the allocation


      // point grandparent to new free location
        grandparent->next = dest;
      }else{
        freeHead=dest;
      }
  
  // unlock
      pthread_mutex_unlock(&lock);
      return retPtr;

    }else{
      // unlock
      pthread_mutex_unlock(&lock);
      return NULL;
    }
  }
  if(temp==freeHead){
    bollean=1;
  }

  // copy the node struct below the allocation
  node *const src = temp;
  int newFreeSize = temp->size - totalSize;
  temp->size = newFreeSize;
  node *dest = (node *)((char *)temp + totalSize);
  memmove(dest, src, sizeof(node));
  //input the meta data for the allocation block
  header *allocatedHead = (header *)temp;
  allocatedHead->size = totalSize-headSize;
  allocatedHead->magicNumber = MN;

  if(bollean==1){
    freeHead = dest;
  }else{
    parent->next = dest;
  }
  
    myassert(allocatedHead->magicNumber == MN);

  void *retPtr = (void *)((char *)allocatedHead + headSize);
  // unlock
  pthread_mutex_unlock(&lock);
  return retPtr;
   

}


/* myfree: unallocates memory that has been allocated with mymalloc.
     void *ptr: pointer to the first byte of a block of memory allocated by 
                mymalloc.
     retval: 0 if the memory was successfully freed and 1 otherwise.
             (NOTE: the system version of free returns no error.)
*/
unsigned int myfree(void *ptr) {
  // lock the critical session
  
  pthread_mutex_lock(&lock);
  // get the meta date of *ptr
  header *toFreeHeader = (header *)((char *)ptr - sizeof(header));
  // int freeSize = toFreeHeader->size + sizeof(header);

  node *head = freeHead;
  int compare = (char *)ptr-(char *)head;
  if(compare<0){
    node *newHead = extendFreeListBeforeHead(head, toFreeHeader);
    if(newHead !=NULL && (char *)newHead == (char *)toFreeHeader){
      freeHead = newHead;
      // unlock
      pthread_mutex_unlock(&lock);
      return 0;
    }
  }else{
    int ok;
    ok = extendFreeListAfterHead(head, toFreeHeader); // FIXME
    // unlock
    pthread_mutex_unlock(&lock);
    return ok;
  }
  // unlock
  pthread_mutex_unlock(&lock);
	// placeholder so that the program will compile
  return 1;
}

unsigned int extendFreeListAfterHead(node *head, header *toFree){
  node *parent;
  node *temp=head;
  int compare = (char *)toFree - (char *)temp;
  while(compare>0 && temp!=NULL){
    parent = temp;
    temp = temp->next;
    compare = (char *)toFree - (char *)temp;
  }
  // node *parentHead = (node *)((char *)parent - sizeof(node));
  // int parentTotalSize = parentHead->size + sizeof(node);
  // header *toFreeHeader = (header *)((char *)toFree - sizeof(header));
  // int freeSize = toFreeHeader->size + sizeof(header);
  //insert before the bottom node
  node *tempHead = extendFreeListBeforeHead(temp, toFree);
  if((parent->size + (char *)parent + sizeof(node))==(char *)tempHead){
    parent->size += tempHead->size + sizeof(node);
    parent->next = tempHead->next;
    return 0;
  }else{
    parent->next = tempHead;
    return 0;
  }
  return 1;
  
}

node *extendFreeListBeforeHead(node *ListHead, header *toFreeHeader){
    int freeSize = toFreeHeader->size + sizeof(header);
    myassert(toFreeHeader->magicNumber == MN);
    node *nodeHead = (node *)toFreeHeader;
    if(((char *)toFreeHeader + freeSize)==(char *)ListHead){
      nodeHead->size = freeSize + ListHead->size;
      nodeHead->next = ListHead->next;

    }else{
      nodeHead->size = freeSize-sizeof(node);
      nodeHead->next = ListHead;
    }

    return nodeHead;
}


int mymalloc_increase(unsigned int size) {
  // placeholder so that the program will compile
  
  if(sbrk(size)!=(void *) -1){
    return 0;
  }
  return 1; // non-zero return value indicates an error
}



