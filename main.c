//https://arjunsreedharan.org/post/148675821737/memory-allocators-101-write-a-simple-memory

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h> //for boolean datatype
//type define size 16byte space for memory
typedef char ARIS[16];
// union of structor s containg information like size , next memory block pointer , is_free or not ;
union header{
    struct 
    {
        size_t size;
        bool is_free;
        //not using pointer instead union pointer
        union header *next;
    }s;
    ARIS stub; //stub temp. space occupied of 16 bytes
    
};
//making the union header (short to ) --->  hrader_t;
typedef union header header_t;

//Global variables for header and tail 
header_t *head,*tail;

//multithread protection (idk how it works)
pthread_mutex_t global_malloc_lock;



//sample malloc function 
/*
void *malloc(size_t size){
    //pointer to the start of the block of memory
    void *block;
    block = sbrk(size); // create space for the required size 
    if(block == (void*)-1 ) {
        //error
        return NULL;
    }
    return block;//but it is not guranteed that we get memory in contnuous form therefore we'll use header and tail to form a memory allocator in the fom of a linked list 

}
*/
//modified malloc with multithread protection and free spacke checker from existing free fragments of memory
//get space from pre allocated free memory blocks 
header_t *get_free_block(size_t size){
    header_t *curr = head;
    while(curr){
        if(curr->s.is_free && curr->s.size >= size){
            return curr;
        }
        curr = curr->s.next;
    }
    return NULL;
}
void *malloc(size_t size){
    //first find if required space is available from the is_free == true block of memory else take from the os 
    //calc total size if need to fectch mem from os
    size_t total_size = size + sizeof(header_t);
    void *block;
    header_t *header;
    if(!size){
        return NULL;
    }
    //multithread bhang bhoseda
    pthread_mutex_lock(&global_malloc_lock);
    header = get_free_block(size); //getes the desire block of size ; if available from is_free== true blocks
    if(header){ //if found space define the things ion the header
        header->s.is_free = false;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void*)(header+1);//return the new brk 
    }
    //if not available get from os
    block = sbrk(total_size);
    if(block == NULL){
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }
    header = block;
    header->s.is_free = false;
    header->s.size = size;
    header->s.next = NULL;

    //now point this new spaces header to the tail of the previous memory block 

    if(!head){
        head = header; //head point to the very first block of memory allocted 
    }
    if(tail){
        tail->s.next = header; //this is the tail of the previous block
    }
    tail = header; //now it is the tail of this memory block 
    pthread_mutex_unlock(&global_malloc_lock);
    return (void*)(header+1);
}



//now i need to free memory if it is at the top else mark the mem locations as is_free = true
void free(void *block){
    header_t *header,*temp;
    void *programbreak;

    if(!block){
        return ;
    }
    pthread_mutex_lock(&global_malloc_lock);
    header = (header_t*)block-1;

    programbreak = sbrk(0); //get the current program break
    //if the block to be freed is at the end of the heap
    if((char*)block + header->s.size == programbreak){
        if(head == tail){ //if only one block is present
            head = tail = NULL;
        }else{
            temp = head;
            while(temp){
                if(temp->s.next == tail){
                    temp->s.next = NULL;
                    tail = temp;   
                }
                temp = temp->s.next;
            }
        }
        sbrk(0 - (header->s.size + sizeof(header_t))); //reduce the program break
        pthread_mutex_unlock(&global_malloc_lock);
        return;
    }
    //if not at the end of the heap mark the block as free
    header->s.is_free = true;
    pthread_mutex_unlock(&global_malloc_lock);
    return;
}

//calloc
void *calloc(size_t num,size_t nsize){
    size_t size;
    void *block;
    if(!num || !nsize){
        return NULL;
    }
    size = num * nsize;
    if(nsize != size/num){ //overflow check
        return NULL;    }
    block = malloc(size);
    if(!block){
        return NULL;
    }
    memset(block,0,size); //set the allocated memory to zero
    return block;
    
}
void *realloc(void *block, size_t size)
{
	header_t *header;
	void *ret;
	if (!block || !size)
		return malloc(size);
	header = (header_t*)block - 1;
	if (header->s.size >= size)
		return block;
	ret = malloc(size);
	if (ret) {
		
		memcpy(ret, block, header->s.size);
		free(block);
	}
	return ret;
}

void print_mem_list()
{
	header_t *curr = head;
	printf("head = %p, tail = %p \n", (void*)head, (void*)tail);
	while(curr) {
		printf("addr = %p, size = %zu, is_free=%u, next=%p\n",
			(void*)curr, curr->s.size, curr->s.is_free, (void*)curr->s.next);
		curr = curr->s.next;
	}
}

int main(){
   printf("=== Custom Simple Memory Allocator Test ===\n");

    int *a = (int *)malloc(sizeof(int));
    *a = 100;
    printf("Allocated a = %p, value = %d\n", (void*)a, *a);
    print_mem_list();

    int *b = (int *)malloc(4 * sizeof(int));
    for (int i = 0; i < 4; i++) b[i] = i * 10;
    printf("Allocated b = %p\n", (void*)b);
    print_mem_list();

    free(a);
    printf("Freed a = %p\n", (void*)a);
    print_mem_list();

    int *c = (int *)malloc(sizeof(int));
    *c = 999;
    printf("Allocated c = %p, value = %d\n", (void*)c, *c);
    print_mem_list();  // Should reuse freed block `a`

    free(b);
    free(c);

    void *d = calloc(5, sizeof(int));
    printf("Calloc'd d = %p\n", d);
    print_mem_list();

    d = realloc(d, 10 * sizeof(int));
    printf("Reallocated d = %p\n", d);
    print_mem_list();

    free(d);
    print_mem_list();

    return 0;
}
