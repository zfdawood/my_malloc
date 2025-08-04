/* THIS CODE WAS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING ANY
SOURCES OUTSIDE OF THOSE APPROVED BY THE INSTRUCTOR. Zachariah Dawood */
//
//  my_malloc.c
//  Lab1: Malloc
//

#include "my_malloc.h"
#include <unistd.h>
#define MAGICNO 261311
MyErrorNo my_errno=MYNOERROR;
FreeListNode start = NULL;


void *my_malloc(uint32_t size){
    // determine the size of the chunk (the original specified size plus the size of the 
    // 8 byte header plus padding so that it aligns to a multiple of 8)
    // if size is zero, set mychunksize to 16 since that's the minimum and the
    // forumula below would calculate 8 for an input of 0
    int mychunksize = size == 0 ? 16 : (size + 8 + (size % 8 == 0 ? 0 : 8 - (size % 8))); 
    // if the pointer to the start of the free list isn't null, it is necessary to
    // search through the free list to see if there's anything that would fit
    // this is done by keeping track of the current node and the previous node and
    // following the linked list, assigning current = current -> flink at the end of the
    // iteration and assigning previous to current.
    if(start != NULL){
        FreeListNode current = start, prev = NULL;
        while(current != NULL){
            // if a large-enough chunk is found (the size of the chunk is greater than or
            // equal to the new chunk's needed size), a couple things happen: 
            // another pointer to the same address is created but as an integer to make 
            // assigning the size and magic number easier, the next entry in the linked list
            // is tracked since the pointer to it from the current node will be lost when 
            // overwritten and similarly the old size of the chunk is kept track of so that 
            // it can be compared with the mychunksize to see if splitting the chunk is necessary 
            // or possible
            if(mychunksize <= current -> size){
                int* reusedptr = (int*) current;
                FreeListNode next = current -> flink;
                int oldchunksize = current -> size;
                // if there is enough room in old chunk to split into the requested chunk
                // and another one (of size at least 16). Usincg the int pointer, the 
                // first 4 bytes are used to hold the mychunksize (which the first part
                // will give exactly) and the second 4 bytes are used to hold the magic number
                // used by free to ensure that its actually freeing something created with 
                // my_malloc. A new freelistnode pointer is created that points to mychunksize bytes
                // after the address that gets malloced. From the second part, the pointer to 
                // the old flink is assigned to the new free list node's flink since the new
                // free list's address is exactly between the current one's (that is now being
                // reallocated) and its old flink. The size is also assigned, which is the remainer
                // of the oldchunksize after mychunksize is taken out of it. 
                if(mychunksize + 16 <= oldchunksize){
                    reusedptr[0] = mychunksize;
                    reusedptr[1] = MAGICNO;
                    FreeListNode secondpart = (FreeListNode)(((void*) reusedptr) + mychunksize);
                    secondpart -> flink = next;
                    secondpart -> size = oldchunksize - mychunksize;
                    // if the current node (that is now being reallocated) is the start,
                    // assign the second part directly to stop. This is necessary because
                    // prev is initally assigned to NULL so this prevents the pointer to the
                    // second part from being lost.
                    if(current == start){
                        start = secondpart;
                    // if prev is actually a node, its flink is changed from the current node
                    // to the second part after the split.
                    } else {
                        prev -> flink = secondpart;
                    }
                } else {
                    // if there is not enough space to split the chunk, a very similar process
                    // as above happens when it comes to assigning size and the magic number but no
                    // second part is allocated.
                    reusedptr[0] = oldchunksize;
                    reusedptr[1] = MAGICNO;
                    if(current == start){
                        start = next;
                    } else {
                        prev -> flink = next;
                    }
                }
                // move up 8 bytes so the user isn't overwriting the chunk header and then
                // return the pointer. 
                return ((void*) reusedptr) + 8;
            }
            prev = current;
            current = current -> flink;
        }
    }
    // if start is null or if there is no appropriately-sized entry in the free list
    // sbrk is called with 8192 bytes or, if the requested byte size is larger, mychunksize
    // (recall that the first line of this function esures that no matter what, sbrk is called 
    // on a multiple of 8). If sbrk fails, the error number is set and null is returned. 
    void* sbrkoutput = sbrk(mychunksize < 8192 ? 8192 : mychunksize);
    if(sbrkoutput == (void*) -1){
        my_errno = MYENOMEM;
        return NULL;
    }
    // the rest of this is very similar to how its allocated from the free list
    // with the cast to int to set chunksize and the magic number. 
    int* mychunkptr = (int*) sbrkoutput;
    mychunkptr[0] = mychunksize;
    mychunkptr[1] = MAGICNO;
    // as before, if there is room in the chunk (which only happens if 
    // 8192 bytes are requested), a new entry is added to the free list that is
    // mychunksize bytes from the pointer returned by sbrk. If start is null, that
    // gets appended to the beginning of the list with size 8192 - mychunksize and its
    // flink will always be null since this part of the code is only accessible if there is
    // no free list or if all entries have been exhausted. If it doesn't append to the start, then it
    // appends to the end so the list must be traversed and the second part gets added to the last node's
    // flink. 
    if(mychunksize + 16 <= 8192){
        void* mychunkptrvoid = (void*) mychunkptr;
        mychunkptrvoid += mychunksize;
        FreeListNode toappend = (FreeListNode) mychunkptrvoid;
        if(start == NULL){
            start = toappend;
            start -> flink = NULL;
            start -> size = 8192 - mychunksize;
        } else {
            FreeListNode current = start;
            while(current -> flink != NULL) current = current -> flink;
            current -> flink = toappend;
            current -> flink -> flink = NULL;
            current -> flink -> size = 8192 - mychunksize;
        }
    // if it is not large enough the pointer will be oversized and the size is updated
    // to reflect that. 
    } else if (mychunksize <= 8192){
        mychunkptr[0] += 8192 - mychunksize;
    }
    // return the first useable byte of the malloced pointer.
    return ((void*) mychunkptr) + 8;
}

void my_free(void *ptr){
    // if the magic number one byte before the useable portion of the pointer is not
    // the expected one or the ptr is null, set the error number and return;
    if(((int*)ptr)[-1] != MAGICNO || ptr == NULL){
        my_errno = MYBADFREEPTR;
        return;
    }
    // moves the pointer back to the beginning of the chunk header for ease of calculation
    // and stores the size that's newly freed. If start is null and there are no free list nodes,
    // the pointer to the newly-freed space is assigned to start and the first free list node
    // are established at that address and the fuction returns.
    ptr -= 8;
    int freedsize = ((int*)ptr)[0];
    if(start == NULL){
        start = ptr;
        start -> flink = NULL;
        start -> size = freedsize;
        return;
    }
    // if the freed pointer lower than the old start, it should be the first entry in the free list
    // so start is assigned to the pointer and the old start is the flink from the new one.
    FreeListNode current = start;
    if((void*) ptr < (void*) start){
        start = ptr;
        start -> flink = current;
        start -> size = freedsize;
        return;
    }
    // this while loop determines where in the free list the new entry should be. If the current
    // node's flink's address is greater than the input pointer's address, the new node should go
    // between the current node and the next one and the loop breaks (that case is taken care of
    // below) 
    while((void*) current -> flink < (void*) ptr){
        // if the last entry in the list is encountered, the new free list node 
        // should then be the last entry in the list and so the old last entry's flink should be
        // set to the pointer, which marks the new last entry and its size and pointer to NULL are set
        if(current -> flink == NULL){
            current -> flink = ptr;
            current -> flink -> flink = NULL;
            current -> flink -> size = freedsize;
            return;
        }
        // iterate through the loop
        current = current -> flink;
    }
    // if the new node belongs in the middle of the loop after a certain current node
    // save the current node's old flink and assign the new pointer to it and then assign the
    // old flink to the new node's new flink
    FreeListNode next = current -> flink;
    current -> flink = ptr;
    current -> flink -> flink = next;
    current -> flink -> size = freedsize;
}

// just a getter for the start global variable that cointains the pointer to the
// start of the free list. If there is no free list, this will return null since that's what
// start is initially assigned to. 
FreeListNode free_list_begin(){
    return start;
}

// if start is null, there are no entries in the free list and therefore nothing to coalesce
// otherwise, go through the list. If the pointer to the current entry plus its size in bytes 
// is the pointer to the the next entry (which is the closest entry that whose pointer is greater
// than it), then merge them by adding their sizes in the current node's size field and 
// setting the current entry's flink to its old successor's flink. Current is not updated in this case
// so if there is another entry in the free list that directly follows the now-larger current entry
// the same process can be applied to absorb it. If the memory is not contigious, go to the
// next entry in the list.
void coalesce_free_list(){
    if(start == NULL) return;
    FreeListNode current = start;
    while(current -> flink != NULL){
        if(((void*)current) + current -> size == (void*)current -> flink){
            current -> size = current -> size + current -> flink -> size;
            current -> flink = current -> flink -> flink;
        } else {
            current = current -> flink;
        }
    }
}
