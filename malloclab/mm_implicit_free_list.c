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
    "self-learner",
    /* First member's full name */
    "Ethan Teh",
    /* First member's email address */
    "tehzl@hotmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE 4

/* My preprocessor macros */
#define WSIZE SIZE_T_SIZE
#define DSIZE (2 * SIZE_T_SIZE)
#define MIN_BLK_SIZE (2 * DSIZE)
#define ADJUST_SIZE(size) (((size) <= DSIZE ? DSIZE : ALIGN(size)) + DSIZE)
#define CHUNKSIZE (1<<13)
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x7)
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define PREV_FTRP(bp) ((char *)(bp) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(PREV_FTRP(bp)))

/* A pointer always point to the prologue block*/
static void* heap_listp;
static int mm_checker;

/* function declaration in this file */
int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);
static void *extend_heap(size_t size);
static void *coalesce(void *ptr);
static void *find_fit(size_t size);
static void place(void *ptr, size_t size);
static void print_block_status(void* bp);
static void print_block_status_verbose(void* bp);
static int is_valid_block_pointer(void* bp);
static int mm_check(void);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    mem_init();
    mm_checker = 1;
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) {
        return -1;
    };
    PUT(heap_listp, 0);
    PUT(heap_listp + WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + (WSIZE * 2), PACK(DSIZE, 1));
    PUT(heap_listp + (WSIZE * 3), PACK(0, 1));
    heap_listp += DSIZE;

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        fprintf(stderr, "extend_heap: extension error\n");
        return -1;
    }
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize, extended_size;
    void *bp;

    asize = ADJUST_SIZE(size);

    if ((bp = find_fit(asize)) == NULL) {
        extended_size = MAX(asize, CHUNKSIZE);
        if ((bp = extend_heap(extended_size / WSIZE)) == NULL) {
            return NULL;
        }
    }
    // printf("malloc: %8x, %8x\n", bp, asize);
    
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    // printf("free: %8x\n", ptr);
    if (!is_valid_block_pointer(ptr)) return;
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(FTRP(ptr), PACK(size, 0));
    PUT(HDRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc_ver1(void *ptr, size_t size)
{
    void *new_ptr;
    size_t old_size, new_size;
    ssize_t size_diff;

    //special cases
    if (ptr == NULL) {
        return mm_malloc(size);
    }
    if (size == 0) {
        mm_free(ptr);
        return (void *)-1;
    }
    old_size = GET_SIZE(HDRP(ptr));
    new_size = ADJUST_SIZE(size);
    new_ptr = mm_malloc(size);
    old_size = old_size > new_size ? new_size : old_size;
    for (int i = 0; i < (old_size - DSIZE)/WSIZE; i++) {
        PUT((unsigned int*)new_ptr + i, GET((unsigned int*)ptr + i));
    }
    mm_free(ptr);
    return new_ptr;
}

void *mm_realloc_ver2(void *ptr, size_t size)
{
    void *new_ptr, *next_ptr, *next_hdr;
    size_t old_size, new_size;
    ssize_t size_diff;

    //special cases
    if (ptr == NULL) {
        return mm_malloc(size);
    }
    if (size == 0) {
        mm_free(ptr);
        return (void *)-1;
    }

    old_size = GET_SIZE(HDRP(ptr));
    new_size = ADJUST_SIZE(size);
    size_diff = new_size - old_size;
    if (size_diff <= 0) {
        place(ptr, new_size);
    } else {    
        next_ptr = NEXT_BLKP(ptr);
        next_hdr = HDRP(next_ptr);

        if ( !GET_ALLOC(next_hdr) && GET_SIZE(next_hdr) >= size_diff) {
            PUT(HDRP(ptr), PACK(GET_SIZE(next_hdr) + old_size, 0));
            place(ptr, new_size);
        } else {
            new_ptr = mm_malloc(size);
            if (old_size > new_size) old_size = new_size;
            for (int i = 0; i < (old_size - DSIZE); i++) {
                PUT(new_ptr + i, GET(ptr + i));
            }
            mm_free(ptr);
            ptr = new_ptr;
        }
    }
    return ptr;
}

void *mm_realloc(void *ptr, size_t size)
{
    return mm_realloc_ver2(ptr, size);
}
/* 
 * extend_heap - Extends the heap by `words` words. Returns pointer to the free block if succeed, and returns NULL if failed.
 */
static void *extend_heap(size_t words)
{
    void *bp;
    size_t size;
    mm_check();
    size = ((words + 1) & ~0x1) * WSIZE;
    if ((bp = mem_sbrk(size)) == (void *)-1) {
        mm_check();
        printf("hi: %8x, lo: %8x\n", mem_heap_hi(), mem_heap_lo());
        printf("diff: %x\n", mem_heap_hi()-mem_heap_lo());
        return NULL;
    }
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    bp = coalesce(bp);
    return bp;
}

/*
 * coalesce - coalesce the next block(and the previous block) to the free block bp points at if the block is free. Returns the pointer to the coalesced free block.
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(PREV_FTRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
    }

    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) {
        bp = PREV_BLKP(bp);
        size += GET_SIZE(HDRP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        bp = PREV_BLKP(bp);
        size += GET_SIZE(HDRP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    // mm_checker = mm_check();
    return bp;
}

/*
 * (static) find_fit - find a free block with at least asize bytes
 */
static void *find_fit(size_t asize) 
{
    void *blkp = heap_listp;
    for (; GET_SIZE(HDRP(blkp)) != 0; blkp = NEXT_BLKP(blkp)) {
        if (GET_ALLOC(HDRP(blkp)) == 0 && GET_SIZE(HDRP(blkp)) >= asize) {
            return blkp;
        }
    }
    return NULL;
}

/*
 * (static) place - allocate a block of `size` bytes(including header and footer), which bp point at.
 *      Doesn't check if the allocation is valid. Make sure that the block bp point at has at least `size` bytes.
 */
static void place(void* bp, size_t size)
{
    size_t bp_size = GET_SIZE(HDRP(bp));

    if ((bp_size - size) < MIN_BLK_SIZE) {
        PUT(HDRP(bp), PACK(bp_size, 1));
        PUT(FTRP(bp), PACK(bp_size, 1));
    }
    else {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(bp_size - size, 0));
        PUT(FTRP(bp), PACK(bp_size - size, 0));
    }
    // mm_checker = mm_check();
}

int mm_check(void) {
    int end_status = 1;
    void *bp;
    for (bp = heap_listp; GET(HDRP(bp)) != end_status; bp = NEXT_BLKP(bp)) {
        if (GET(HDRP(bp)) != GET(FTRP(bp))) {
            printf("error:\n");
            print_block_status_verbose(bp);
            return 0;
        }
    }
    return 1;
}

static void print_block_status(void* bp)
{
    printf("bp: %8x, ", bp);
    printf("size: %8u, ", GET_SIZE(HDRP(bp)));
    printf("alloc: %d\n", GET_ALLOC(HDRP(bp)));
}

static void print_block_status_verbose(void* bp)
{
    printf("check block:%x\n", bp);
    printf("header_status: 0x%x\n", GET(HDRP(bp)));
    printf("footer_status: 0x%x\n", GET(FTRP(bp)));
    printf("size: %u\n", GET_SIZE(HDRP(bp)));
    printf("alloc: %d\n\n", GET_ALLOC(HDRP(bp)));
}

static int is_valid_block_pointer(void* bp) {
    if ((size_t) bp <= (size_t) heap_listp) {
        return 0;
    }
    for(void* blkp = heap_listp; GET(HDRP(blkp)) != 1; blkp = NEXT_BLKP(blkp)) {
        if ((size_t) bp < (size_t) blkp) return 0;
        else if (bp == blkp) return 1;
    }
    return 0;
}





