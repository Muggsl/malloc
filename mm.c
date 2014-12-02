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
    /* Team name */
    "alan+sam",
    /* First member's full name */
    "Sam Elberts",
    /* First member's email address */
    "samuelelberts2015@u.northwestern.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

    #define DSIZE 8             // Double word size (bytes)
    #define CHUNKSIZE (1<<12)   // Amount (bytes) to extend heap by
/* Maximum comparison macro */
    #define MAX(x, y) ((x) > (y)? (x) : (y))
/* Read/Write word from address 'p' */
    #define GET(p)       (*(unsigned int *)(p))
    #define PUT(p, val)  (*(unsigned int *)(p) = (val))
/* Combine size and allocated bit to store in header/footer */
    #define PACK(size, alloc)  ((size) | (alloc))
/* Read size and allocated bit fields of a header/footer at address 'p' */
	#define GET_SIZE(p)  (GET(p) & ~0x7)
	#define GET_ALLOC(p) (GET(p) & 0x1)
/* Compute the address of the header and footer of a block w/ pointer 'ptr' */
	#define HDRP(ptr)       ((char *)(ptr) - ALIGNMENT)
	#define FTRP(ptr)       ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)
/* Compute address of the header and footer of a block given its pointer 'ptr' */
	#define HDRP(ptr)       ((char *)(ptr) - ALIGNMENT)
	#define FTRP(ptr)       ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)
/* Compute address of next and previous blocks for a block given its pointer 'ptr' */
	#define NEXT(ptr)  ((char *)(ptr) + GET_SIZE(((char *)(ptr) - ALIGNMENT)))
	#define PREV(ptr)  ((char *)(ptr) - GET_SIZE(((char *)(ptr) - DSIZE)))
/* Global variables */
	static char *heapPtr = 0;       // Pointer to first block
	static char *searchPtr;			// Location for next fit search
/* Helper function prototypes */
	static void *coalesce(void *ptr);
	static void *extend_heap(size_t words);
    static void  place(void *ptr, size_t asize);
    static void *find_fit(size_t asize);
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Initialize the heap. If out of memory, return error  */
	if ((heapPtr = mem_sbrk(4*ALIGNMENT)) == (void *)-1)
		return -1;
/* Create an empty heap with padding and prologue & epilogue H/F */
	PUT(heapPtr, 0);						    // Alignment padding
	PUT(heapPtr + (1*ALIGNMENT), PACK(DSIZE, 1));	// Prologue header
	PUT(heapPtr + (2*ALIGNMENT), PACK(DSIZE, 1));	// Prologue footer
	PUT(heapPtr + (3*ALIGNMENT), PACK(0, 1));		// Epilogue header (end of heap)
/* Update heap pointers */
    heapPtr += (2*ALIGNMENT);       // Update heap pointer to point to before last block
    searchPtr = heapPtr;        // Initialize location for next fit search
/* Extend the empty heap with a free block of 'CHUNKSIZE' bytes */
	if (extend_heap(CHUNKSIZE/ALIGNMENT) == NULL)
		return -1;
	return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	size_t asize;				// Adjusted block size
	size_t extendsize;			// Amount to extend heap if no fit
	char *ptr;                  // Pointer to allocation point
/* Ignore erroneous requests with size=0 */
	if (size == 0) return NULL;
/* Adjust block size to include header/footer and alignment requirement */
	if (size <= DSIZE)
		asize = 2*DSIZE;
	else
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
/* Search free list for a fit using the next-fit algorithm */
	if ((ptr = find_fit(asize)) != NULL)    // If there is a fit...
	{
		place(ptr, asize);  // Place the requested block and split if necessary
		return ptr;         // Return the address of newly allocated block
	}
/* No fit found for size. Extend the heap size and place the block */
	extendsize = MAX(asize,CHUNKSIZE);      // Size of extension
    ptr = extend_heap(extendsize/ALIGNMENT);    // Set ptr to address of new block
	if (ptr == NULL) return NULL;           // Return NULL if memory error
    place(ptr, asize);      // Place the requested block and split if necessary
	return ptr;             // Return the new block pointer

}

/*
 * mm_free - free a previously allocated block (bp) and merge
 *           adjacent free blocks using boundary-tags coalescing
 */
void mm_free(void *bp)
{
	if(bp == 0)
		return;

	size_t size = GET_SIZE(HDRP(bp));

	if (heapPtr == 0){
		mm_init();
    }

	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));

    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/*
 * extend_heap - Extends the heap size: create a new free block at the end
 *               of the heap and move the epilogue to the new heap end
 */
static void *extend_heap(size_t words)
{
	char *ptr;      // Create a new pointer
	size_t size;    // Create a size variable
/* Allocate an even number of words to maintain alignment */
	size = (words % 2) ? (words+1) * ALIGNMENT : words * ALIGNMENT;
	if ((long)(ptr = mem_sbrk(size)) == -1) // 'ptr' points to new block
		return NULL;                        // Memory error
/* Initialize free block header/footer and the epilogue header */
	PUT(HDRP(ptr), PACK(size, 0));			// Free block header
	PUT(FTRP(ptr), PACK(size, 0));			// Free block footer
	PUT(HDRP(NEXT(ptr)), PACK(0, 1));	    // New epilogue header
/* Coalesce if the previous block was free */
	return coalesce(ptr);
}

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));  // Old size of the block

    if ((csize - asize) >= (2*DSIZE))   // If there is room to split, split
    {
	    PUT(HDRP(bp), PACK(asize, 1));
	    PUT(FTRP(bp), PACK(asize, 1));
	    bp = NEXT(bp);
	    PUT(HDRP(bp), PACK(csize-asize, 0));
	    PUT(FTRP(bp), PACK(csize-asize, 0));
    }
    else        // there is no room to split, place the block
    {
	    PUT(HDRP(bp), PACK(csize, 1));
	    PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * find_fit - Find a fit using next-fit search for a block of asize bytes
 */
static void *find_fit(size_t asize)
{
    char *oldsearchPtr = searchPtr;
/* Search from the search location to the end of list */
    for ( ; GET_SIZE(HDRP(searchPtr)) > 0; searchPtr = NEXT(searchPtr))
	if (!GET_ALLOC(HDRP(searchPtr)) && (asize <= GET_SIZE(HDRP(searchPtr))))
	    return searchPtr;   // Fit found, return search pointer
/* Search from start of list to old searchPtr */
    for (searchPtr = heapPtr; searchPtr < oldsearchPtr; searchPtr = NEXT(searchPtr))
	if (!GET_ALLOC(HDRP(searchPtr)) && (asize <= GET_SIZE(HDRP(searchPtr))))
	    return searchPtr;   // Fit found, return search pointer
    return NULL;            // Fit not found, return NULL
}

/*
 * coalesce - Coalescing function using boundry tags. Merges adjacent free blocks
 *            (4 possible cases described in if statement)
 */
static void *coalesce(void *ptr)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV(ptr)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT(ptr)));
	size_t size = GET_SIZE(HDRP(ptr));
/* Case 1: prev and next allocated */
	if (prev_alloc && next_alloc)
	{
		return ptr;
	}
/* Case 2: prev allocated, next free */
	else if (prev_alloc && !next_alloc)
	{
		size += GET_SIZE(HDRP(NEXT(ptr)));
		PUT(HDRP(ptr), PACK(size, 0));
		PUT(FTRP(ptr), PACK(size,0));
	}
/* Case 3: prev free, next allocated */
	else if (!prev_alloc && next_alloc)
	{
		size += GET_SIZE(HDRP(PREV(ptr)));
		PUT(FTRP(ptr), PACK(size, 0));
		PUT(HDRP(PREV(ptr)), PACK(size, 0));
		ptr = PREV(ptr);
	}
/* Case 4: next and prev free. */
	else
	{
		size += GET_SIZE(HDRP(PREV(ptr))) + GET_SIZE(FTRP(NEXT(ptr)));
		PUT(HDRP(PREV(ptr)), PACK(size, 0));
		PUT(FTRP(NEXT(ptr)), PACK(size, 0));
		ptr = PREV(ptr);
	}
/* Adjust next-fit search location if it points to new free block */
    if ((searchPtr > (char *)ptr) && (searchPtr < NEXT(ptr)))
	    searchPtr = ptr;
	return ptr;
}












