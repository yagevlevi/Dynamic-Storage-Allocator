/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* bu username : eg. jappavoo */
    "yagev",
    /* full name : eg. jonathan appavoo */
    "yagev levi",
    /* email address : jappavoo@bu.edu */
    "yagev@bu.edu",
    "",
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros*/
#define WSIZE 4 /* Word and Header/Footer size */
#define DSIZE 8
#define CHUNK (1<<12) /* Used to extend the heap by this amount of bytes */
#define PACK(size, alloc) ((size | alloc)) /* Pack a size and alloacted word bit into a word */
#define READ(p) (*(unsigned int *)(p)) /* Read a word at address p */
#define WRITE(p, val) (*(unsigned int *)(p) = (val)) /* Write a word at address p */
#define READ_SIZE(p) (READ(p) & ~0x7) /* Read the size field from address p */
#define READ_ALLOC(p) (READ(p) & 0x1) /* Read the allocated field from address p */
#define HDRP(bp) ((char *)(bp) - WSIZE) /* Computes the address of the header of a given block pointer */
#define FTRP(bp) ((char *)(bp) + READ_SIZE(HDRP(bp)) - DSIZE) /* Computes the address of the footer of a given block pointer */
#define NEXT_BLKP(bp) ((char *)(bp) + READ_SIZE(((char *)(bp) - WSIZE))) /* Computes the address of the next block after the block bp points to */
#define PREV_BLKP(bp) ((char *)(bp) - READ_SIZE(((char *)(bp) - DSIZE))) /* Computes the address of the previous block before the block bp points to */
#define MAX(x, y) ((x) > (y)? (x) : (y));
void *heaplistptr;

/*
 * mm_check - checks if the current heap is valid or not, returns a nonzero value if and only if the heap is consistent
 *
*/

int mm_check(void) {
    void *bp = heaplistptr;
    while (1) {
        if (READ_SIZE(HDRP(bp)) != READ_SIZE(FTRP(bp)) || READ_ALLOC(HDRP(bp)) != READ_ALLOC(FTRP(bp))) {
            return -1;
        }
        bp = NEXT_BLKP(bp);
    }
    return 1;
}

/*
 * coalesce - combines consecutive free blocks into one free block
 *
*/

static void *coalesce(char *bp) {

    size_t prev_block = READ_ALLOC(FTRP(PREV_BLKP(bp))); // Checks if previous block is allocated (1) or free (0)
    size_t next_block = READ_ALLOC(HDRP(NEXT_BLKP(bp))); // Checks if next block is allocated (1) or free (0)
    size_t bpsize = READ_SIZE(HDRP(bp)); // Store the size of what the free block should be

    if (prev_block && next_block) { // Case 1: both previous and next block are allocated, no need to coalesce
        return bp;
    } else if (prev_block && !next_block) { // Case 3: next block is free but previous block is allocated, coalesce next and bp
        bpsize += READ_SIZE(HDRP(NEXT_BLKP(bp))); // Add the size of the next block to the current size to get the size of the coalesced free block
        WRITE(HDRP(bp), PACK(bpsize, 0)); // Overwrite the header of the current block with the new size and free alloc bit
        WRITE(FTRP(bp), PACK(bpsize, 0)); // Overwrite the footer of the next block with the new size and free alloc bit
    
    } else if (!prev_block && next_block) { // Case 2: previous block is free but next block is allocated, coalesce previous and bp
        bpsize += READ_SIZE(HDRP(PREV_BLKP(bp))); // Add the size of the previous block to the current size to get the size of the coalesced free block
        WRITE(FTRP(bp), PACK(bpsize, 0)); // Overwrite the footer of the current block with the new size and free alloc bit
        WRITE(HDRP(PREV_BLKP(bp)), PACK(bpsize, 0)); // Overwrite the header of the previous block with the new size and free alloc bit
        bp = PREV_BLKP(bp); // Sets the new block pointer to the beginning of the coalesced free block which begins where the previous block did
    } else { 
        bpsize += READ_SIZE(HDRP(PREV_BLKP(bp))) + READ_SIZE(FTRP(NEXT_BLKP(bp))) ; // Add the size of the next and previous block to the current size to get the size of the coalesced free block 
        WRITE(HDRP(PREV_BLKP(bp)), PACK(bpsize, 0)); // Overwrite the header of the previous block with the new size and free alloc bit
        WRITE(FTRP(NEXT_BLKP(bp)), PACK(bpsize, 0)); // Overwrite the footer of the next block with the new size and free alloc bit
        bp = PREV_BLKP(bp); // Sets the new block pointer to the beginning of the coalesced free block which begins where the previous block did
    }
    return bp;
}

/*
 * extend_heap - extends the heap with a new free block
 *
*/
static void *extend_heap(size_t words) {
    size_t size; 
    char *bp; 
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; /* Makes sure we are going to allocate an even number of words to maintain alignment*/
    if ((long)(bp = mem_sbrk(size)) == -1) { /* Checks to see if memory was allocated correctly */
        return NULL; 
    }

    WRITE(HDRP(bp), PACK(size,0)); /* Creates the free block header */
    WRITE(FTRP(bp), PACK(size,0)); /* Creates the free block footer that tells us where the memory allocated ends*/
    WRITE(HDRP(NEXT_BLKP(bp)), PACK(0,1)); /* Creates the new epilogue header after the free block footer */

    return coalesce(bp);
}

/*
 * FINDFIT - finds the free block of a specific size using next-fit, assumes size that is passed in is already double word aligned
 *
*/

static void *FINDFIT(size_t size) {
    void *bp;
 
    for (bp = heaplistptr; READ_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) { 
        if (!READ_ALLOC(HDRP(bp)) && (size <= READ_SIZE(HDRP(bp)))) { // Loop scans the heap until it finds a potential fit
            return bp;
        }
    }

    return NULL;
}

/*
 * PLACE - places the block that bp points to at the beginning of the free block
 *
*/

static void PLACE(void *bp, size_t size) {

    size_t csize = READ_SIZE(HDRP(bp));
    size_t diff = csize - size;

    if (diff >= (2*DSIZE)) {
        WRITE(HDRP(bp), PACK(size, 1));
        WRITE(FTRP(bp), PACK(size, 1));
        bp = NEXT_BLKP(bp);
        WRITE(HDRP(bp), PACK(diff, 0));
        WRITE(FTRP(bp), PACK(diff, 0));
    } else {
        WRITE(HDRP(bp),PACK(csize, 1));
        WRITE(FTRP(bp), PACK(csize, 1));
    }
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    /* Creates the inital empty heap and stores a pointer to the first byte */
    if ((heaplistptr = mem_sbrk(4*WSIZE)) == (void *)-1) { /* Checks to see if memory was allocated successfully */
        return -1;
    }

    WRITE(heaplistptr, 0); /* Creates the alignment padding at the begining of the heap */
    WRITE(heaplistptr + (1*WSIZE), PACK(DSIZE, 1)); /* Creates the header for the prologue block */
    WRITE(heaplistptr + (2*WSIZE), PACK(DSIZE, 1)); /* Creates the footer for the prologue block that helps tells us where the beginning of the heap is */
    WRITE(heaplistptr + (3*WSIZE), PACK(0,1)); /* Creates the epilogue header that tells us where the heap ends */
    heaplistptr += (2*WSIZE); /* Increments the pointer to the beginning of the heap, after the epilogue block */

    if (extend_heap(CHUNK/WSIZE) == NULL) { /* Checks to see if more memory can be allocated to the heap */
        return -1;
    }

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {

    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0) { // Ensures we don't do unnecessary work
        return NULL;
    }
    // Adjust the block size as needed
    if (size <= DSIZE) {
        asize = 2*DSIZE;
    } else {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    }
    
    bp = FINDFIT(asize);
    
    // Search the free blocks for a fit
    if (bp != NULL) {
        PLACE(bp, asize); // Place if a fitting free block is found
        return bp;
    }
    
    // If we reach here, that means bp = NULL and we need to extend the heap
    extendsize = MAX(asize, CHUNK);
    bp = extend_heap(extendsize/WSIZE);

    // Check to see if the heap can be extended
    if (bp == NULL) {
        return NULL;
    }
    // Once extended, we can place with confidence
    PLACE(bp, asize);
    return bp;

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {

    size_t size = READ_SIZE(HDRP(ptr));

    WRITE(HDRP(ptr), PACK(size, 0)); // Rewriting the allocated bit as 0 in the header
    WRITE(FTRP(ptr), PACK(size, 0)); // Rewriting the allocated bit as 0 in the footer
    coalesce(ptr); // Calling coalesce incase there are free blocks nearby
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    void *oldptr = ptr;
    void *newptr;
    /*size_t copySize;*/
    size_t oldsize = READ_SIZE(ptr);
    void *bp;
    
    if (ptr == NULL) {
        return mm_malloc(size);
    }

    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    if (size < oldsize) {
        bp = HDRP(ptr);
        size_t old = READ_SIZE(bp);
        WRITE(bp, PACK(size, 1));
        WRITE(bp + size*WSIZE, PACK(size, 1));
        WRITE(bp + size*2*WSIZE, PACK((old-size), 0));
        WRITE(FTRP(ptr), PACK((old-size),0));
        mm_free(ptr);
        return ptr;
    }

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    /*copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;*/
    size_t info = PACK(READ_SIZE(HDRP(newptr)), 1);
    memcpy(newptr, oldptr, size);
    WRITE(HDRP(newptr), info);
    WRITE(FTRP(newptr), info);
    mm_free(oldptr);
    return newptr;
}














