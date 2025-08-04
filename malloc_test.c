#include "my_malloc.h"
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

void *ptrs[10];
void *base;

// Prints the free list
void printFreeList(){
    FreeListNode ptr = free_list_begin();
        unsigned int relAdd = (void*)ptr - base;
    printf("First node at: 0x%x\n", relAdd);
    printf("Size of free chunk: %d\n", ptr->size);
    ptr=ptr->flink;

    while(ptr != NULL){
                relAdd = (void *)ptr - base;
        printf("Next node at: 0x%x\n", relAdd);
        printf("Size of free chunk: %d\n", ptr->size);
        ptr = ptr->flink;
    }
        printf("\n");
}

/* 
    Mallocs several blocks of different sizes and then measures 
    the sizes of the chunks versus the size in the header
*/
void basic_test(){
        printf("Start of Basic Test\n\n");

        unsigned int dist[6];

        ptrs[0] = my_malloc(8);
        ptrs[1] = my_malloc(10);
        ptrs[2] = my_malloc(17);
        ptrs[3] = my_malloc(50);
        ptrs[4] = my_malloc(1);
        ptrs[5] = my_malloc(30);
        ptrs[6] = my_malloc(8);

        for(int i = 0; i < 6; i++){
                dist[i] = ptrs[i+1] - ptrs[i];
                printf("Distance between allocation %u and %u is %u\n", i, i+1, dist[i]);
                printf("Size Recorded: %u\n", *(uint32_t*)(ptrs[i]-8));
                if((unsigned long)ptrs[i] % 8 == 0)
                        printf("Allocation %d is 8-byte aligned\n", i);
                else
                        printf("Allocation %d, is not 8-byte aligned\n", i);

        }
        printf("\nEnd of Basic Test\n\n");
}


/* 
   Frees allocated chunks in arbitraray order and prints the free free list
   after each free
*/
void basic_free(){
        printf("Start of Free Test\n\n");
        my_free(ptrs[0]);
        printFreeList();
        my_free(ptrs[1]);
        printFreeList();
        my_free(ptrs[2]);
        printFreeList();
        my_free(ptrs[6]);
        printFreeList();
        my_free(ptrs[4]);
        printFreeList();
        my_free(ptrs[5]);
        printFreeList();
        my_free(ptrs[3]);
        printFreeList();
        printf("\nEnd of Free Test\n\n");
}

/*
    Allocates memory of the same sizes as previous chunks to make sure
    that the previously free'd nodes are being recycled appropriately
*/
void reclaim(){
        printf("Start of Reclaim Test\n\n");
        ptrs[7] = my_malloc(200); //Nothing appropriate on list.
        ptrs[0] = my_malloc(50);  // Chunk 3
        ptrs[1] = my_malloc(30);  // Chunk 5
        printFreeList();
        ptrs[2] = my_malloc(17);  // Chunk 2
        ptrs[3] = my_malloc(10);  // Chunk 1
        ptrs[4] = my_malloc(8);   // Chunk 0
        ptrs[5] = my_malloc(8);   // Chunk 4
        ptrs[6] = my_malloc(1);   // Chunk 6
        printFreeList();

        my_free(ptrs[1]); // 48 bytes free'd
        printFreeList();
        ptrs[1] = my_malloc(8);  // Chunk 3
        printFreeList();
        ptrs[8] = my_malloc(16);  // Chunk 3.5
        printFreeList();
        printf("\nEnd of Reclaim Test\n\n");
}

/*
    Checks to make sure that the split will not happen if the
    remainder is less than 16 bytes
*/
void internal(){
        printf("Start of Internal Fragmentation Test\n\n");
        my_free(ptrs[0]);  // 64 bytes free'd
        ptrs[0] = my_malloc(48); //This should allocate all 64 bytes.
        printf("Size Recorded: %u\n", *(uint32_t*)(ptrs[0]-8));
        printFreeList();
        printf("\nEnd of Internal Fragmentation Test\n\n");
}

/*
    Frees some consecutive chunks and checks whehter the coalesce 
    is merging them correctly
*/
void combine(){
        printf("Start of Coalescing Test\n\n");
        my_free(ptrs[2]);
        my_free(ptrs[3]);

        my_free(ptrs[1]);
        my_free(ptrs[5]);
        my_free(ptrs[6]);
        my_free(ptrs[8]);

        printFreeList();
        coalesce_free_list();
        printFreeList();

        my_free(ptrs[0]);
        my_free(ptrs[4]);

        coalesce_free_list();
        printFreeList();

        my_free(ptrs[7]);
        printFreeList();
        coalesce_free_list();
        printFreeList();
        printf("\nEnd of Coalescing Test\n\n");
}

/*
    Tests allocating, freeing and coalescing with a large request
*/
void large(){
        printf("Start of Large Allocation Test\n\n");
        ptrs[9] = my_malloc(10001);
        printf("Size Recorded: %u\n", *(uint32_t*)(ptrs[9]-8));
        printFreeList();
        my_free(ptrs[9]);
        printFreeList();
        coalesce_free_list();
        printFreeList();
        printf("\nEnd of Large Allocation Test\n\n");
}

/*
    Calls my_free with a couple of invalid addressees to make sure that
    my_errno is being correclty set for a bad free (based on the detection of 
    the magic number)
*/
void magic(){
        printf("Start of Magic Number Test\n\n");
        printf("Value of errno: %d\n", my_errno);
        my_free(ptrs[0]); // Already free'd
        printf("Value of errno: %d\n", my_errno);
        printFreeList();

        my_errno = 0;
        ptrs[0] = my_malloc(10);
        *(uint32_t*)(ptrs[0]-4) += 1;  // Decrement the magic number
        my_free(ptrs[0]);
        printf("Value of errno: %d\n", my_errno);
        printFreeList();

        my_errno = 0;
        *(uint32_t*)(ptrs[0]-4) -= 1;  // Increment the magic number
        my_free(ptrs[0]);
        printFreeList();

        printf("\nEnd of Magic Number Test\n\n");
}

/*
    Checks that gaps in memory (calling sbrk elsewhere) does not interfere
    with the the allocator
*/
void manual(){
        printf("Start of Direct SBRK Call Test\n\n");
        ptrs[0] = sbrk(1024);
        ptrs[1] = my_malloc(20000);
        my_free(ptrs[1]);
        printFreeList();
        coalesce_free_list();
        printFreeList();
        printf("\nEnd of Direct SBRK Call Test\n\n");

}

/*
    Calls the various tests (in order)
*/
int main(int argc, char const *argv[])
{
        printf(" \b");
        base = sbrk(0);
        basic_test();
        basic_free();
        reclaim();
        internal();
        combine();
        large();
        magic();
        manual();
        return 0;
}