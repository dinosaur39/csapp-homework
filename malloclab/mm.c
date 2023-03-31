/*
 * mm.c - An implementation of memory allocator by using explicit
 *        free list.
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
#define WSIZE SIZE_T_SIZE // size of the address of a byte
#define DSIZE (2 * SIZE_T_SIZE)
#define MIN_BLK_SIZE (2 * DSIZE)
#define ADJUST_SIZE(size) (((size) <= DSIZE ? DSIZE : ALIGN(size)) + DSIZE) // actual size allocated when calling malloc/realloc
#define CHUNKSIZE (1<<12)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PTR_ADD(p, offset) ((void*)(p) + (int)(offset))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x7)
#define PUT(p, val) (*(unsigned int *)(p) = (unsigned int)(val))

#define HDRP(bp) ((bp) - WSIZE)
#define FTRP(bp) ((bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define PREP(bp) (bp)
#define SUCP(bp) ((bp) + WSIZE)
#define PREV_FTRP(bp) ((bp) - DSIZE)
#define NEXT_BLKP(bp) ((bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((bp) - GET_SIZE(PREV_FTRP(bp)))
#define PRED_FREE_BLKP(bp) (void*)(*(unsigned int *)PREP(bp))
#define SUCC_FREE_BLKP(bp) (void*)(*(unsigned int *)SUCP(bp))


static void* heap_listp;// points to the prologue block

/* function declaration in this file */
int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);
static void *extend_heap(size_t size);
static void *coalesce(void *ptr);
static void *find_fit(size_t size);
static void place(void *ptr, size_t size);
static void replace(void *ptr, size_t size, void *pred_blkp, void *succ_blkp);
static void print_block_status(void* bp);
static void print_block_status_verbose(void* bp);
static int mm_check(void);
static void set_allocated_block(void *bp, size_t size);
static void set_free_block(void *bp, size_t size, void *prep, void *sucp);
static void set_block(void *bp, size_t size, void *prep, void *sucp, int alloc);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    void *next_blkp;

    mem_init();
    if ((heap_listp = mem_sbrk(4 * DSIZE)) == (void *)-1) {
        return -1;
    };
    next_blkp = heap_listp + DSIZE * 3;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE * 2, 1));
    PUT(heap_listp + (2 * WSIZE), 0);
    PUT(heap_listp + (3 * WSIZE), next_blkp);
    PUT(heap_listp + (4 * WSIZE), PACK(DSIZE * 2, 1));
    PUT(heap_listp + (5 * WSIZE), PACK(1, 0));
    PUT(heap_listp + (6 * WSIZE), heap_listp + DSIZE);
    PUT(heap_listp + (7 * WSIZE), 0);
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
    if (((unsigned int)ptr % ALIGNMENT) || ptr < heap_listp) return;
    void *pred_free_blk = heap_listp, 
         *succ_free_blk = SUCC_FREE_BLKP(pred_free_blk);

    while(succ_free_blk) {
        if (ptr < succ_free_blk) break;
        pred_free_blk = succ_free_blk;
        succ_free_blk = SUCC_FREE_BLKP(succ_free_blk);
    }
    if (!succ_free_blk || !(GET_ALLOC(HDRP(ptr)))) return;

    set_free_block(ptr, GET_SIZE(HDRP(ptr)), pred_free_blk, succ_free_blk);
    coalesce(ptr);
}

/*
 * mm_realloc_ver1 - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc_ver1(void *ptr, size_t size)
{
    void *new_ptr;
    size_t old_size, new_size;

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
    for (int i = 0; i < (old_size - DSIZE); i+= WSIZE) {
        PUT(PTR_ADD(new_ptr, i), GET(PTR_ADD(ptr, i)));
    }
    mm_free(ptr);
    return new_ptr;
}

void *mm_realloc_ver2(void *ptr, size_t size)
{
    void *new_ptr, *next_ptr, *next_hdr, *pred_blkp, *succ_blkp;
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

    if (size_diff <= (-2 * DSIZE)) {
        pred_blkp = heap_listp;
        succ_blkp = SUCC_FREE_BLKP(pred_blkp);
        while(succ_blkp) {
            if (ptr < succ_blkp) break;
            pred_blkp = succ_blkp;
            succ_blkp = SUCC_FREE_BLKP(succ_blkp);
        }
        if (!succ_blkp) {
            printf("realloc error\n");
            return ptr;
        }
        replace(ptr, new_size, pred_blkp, succ_blkp);
    } else if (size_diff > 0) {    
        next_ptr = NEXT_BLKP(ptr);
        next_hdr = HDRP(next_ptr);
        if (!GET_ALLOC(next_hdr) && GET_SIZE(next_hdr) >= size_diff) {
            pred_blkp = PRED_FREE_BLKP(next_ptr);
            succ_blkp = SUCC_FREE_BLKP(next_ptr);
            set_allocated_block(ptr, old_size + GET_SIZE(next_hdr));
            replace(ptr, new_size, pred_blkp, succ_blkp);
        } else {
            new_ptr = mm_malloc(size);
            if (old_size > new_size) old_size = new_size;
            for (int i = 0; i < (old_size - DSIZE); i += WSIZE) {
                PUT(PTR_ADD(new_ptr, i), GET(PTR_ADD(ptr, i)));
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
    void *bp, *last_blk;
    size_t extend_size;
    extend_size = ((words + 1) & ~0x1) * WSIZE;
    if ((bp = mem_sbrk(extend_size)) == (void *)-1) {
        return NULL;
    }
    bp -= DSIZE; //two words storing the pred and succ address of last block
    set_allocated_block(bp, extend_size);//optimize

    last_blk = NEXT_BLKP(bp);
    set_free_block(bp, extend_size, 0, last_blk);
    set_free_block(last_blk, 0, bp, (void*)-1);

    //mm_check();
    bp = coalesce(bp);
    return bp;
}

/*
 * coalesce - coalesce the next block(and the previous block) to the free block bp points at if the block is free. Returns the pointer to the coalesced free block.
 */
static void *coalesce(void *bp)
{
    int prev_alloc = GET_ALLOC(PREV_FTRP(bp));
    int next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    void* sucp = SUCC_FREE_BLKP(bp);

    if (!next_alloc) {//coalesce current block and next block
        sucp = SUCC_FREE_BLKP(sucp);
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        set_free_block(bp, size, 0, sucp);
    }

    if (!prev_alloc) {//coalesce current block and prev block
        bp = PREV_BLKP(bp);
        size += GET_SIZE(HDRP(bp));
        set_free_block(bp, size, 0, sucp);
    }
    
    //mm_check();
    return bp;
}

/*
 * (static) find_fit - find a free block with at least asize bytes
 */
static void *find_fit(size_t asize) 
{
    void *blkp = SUCC_FREE_BLKP(heap_listp);
    for (; GET_SIZE(HDRP(blkp)) != 0; blkp = SUCC_FREE_BLKP(blkp)) {
        if (GET_SIZE(HDRP(blkp)) >= asize) {
            return blkp;
        }
    }
    return NULL;
}

/*
 * (static) place - allocate a block of `size` bytes(including header and footer), which bp point at.
 *      Doesn't check if the allocation is valid. Make sure that 
 * the block bp point at is free, and has at least `size` bytes.
 */
static void place(void* bp, size_t size)
{
    // size_t bp_size = GET_SIZE(HDRP(bp));
    void *prep = PRED_FREE_BLKP(bp), *sucp = SUCC_FREE_BLKP(bp); 
    replace(bp, size, prep, sucp);
    // if ((bp_size - size) < MIN_BLK_SIZE) {
    //     set_allocated_block(bp, bp_size);
    //     PUT(PREP(sucp), prep);
    //     PUT(SUCP(prep), sucp);
    // }
    // else {
    //     set_allocated_block(bp, size);
    //     bp = NEXT_BLKP(bp);
    //     set_free_block(bp, bp_size - size, prep, sucp);
    //     PUT(PREP(sucp), bp);
    //     PUT(SUCP(prep), bp);
    // }
    // mm_check();
}

/*
 * (static) replace - seperate a block into two parts, the first
 * part is an allocated block of `size` bytes, the second part, 
 * if exists, is a free block.
 * Make sure that the original block has at least `size` bytes.
 * replace() will preserve `size`-DSIZE bytes of data in the 
 * original block. 
 */
static void replace(void *ptr, size_t size, void *pred_blkp, void *succ_blkp)
{
    size_t blk_size = GET_SIZE(HDRP(ptr));
    size_t size_diff = blk_size - size;
    void *new_blk;
    if (size_diff < MIN_BLK_SIZE) {
        set_allocated_block(ptr, blk_size);
        PUT(PREP(succ_blkp), pred_blkp);
        PUT(SUCP(pred_blkp), succ_blkp);
    } else {
        set_allocated_block(ptr, size);
        new_blk = NEXT_BLKP(ptr);
        set_free_block(new_blk, size_diff, pred_blkp, succ_blkp);
    }
}


/*
 * (static) conditioned_put - a helper function of set_block
 */
static void conditioned_put(void *dst, void *val)
{
    if (val) {
        if (val == (void*)(-1)) PUT(dst, 0);
        else PUT(dst, val);
    }
}

/*
 * (static) set_block - a helper function to set the information of a block
 * set prep/sucp to 0 if you don't want to change its value, and set prep/sucp
 * to -1 if you want to reset it to zero.
 */
static void set_block(void *bp, size_t size, void *prep, void *sucp, int alloc)
{
    if (size) { 
        PUT(HDRP(bp), PACK(size, alloc));
        PUT(FTRP(bp), PACK(size, alloc));
    } else { //the last block has no footer
        PUT(HDRP(bp), PACK(0, 1));
    }
    if (!alloc) {
        conditioned_put(PREP(bp), prep);
        conditioned_put(SUCP(bp), sucp);
        prep = PRED_FREE_BLKP(bp);
        sucp = SUCC_FREE_BLKP(bp);
        if (prep) PUT(SUCP(prep), bp);
        if (sucp) PUT(PREP(sucp), bp);
    }
}

static void set_allocated_block(void *bp, size_t size)
{
    set_block(bp, size, 0, 0, 1);
}
static void set_free_block(void *bp, size_t size, void* prep, void *sucp)
{
    set_block(bp, size, prep, sucp, 0);
}

int mm_check(void) {
    int end_status = 1;
    void *bp;
    for (bp = heap_listp; GET(HDRP(bp)) != end_status; bp = NEXT_BLKP(bp)) {
        if (GET(HDRP(bp)) != GET(FTRP(bp))) {
            printf("error of block list:\n");
            print_block_status_verbose(bp);
            return 0;
        }
    }

    for (bp = SUCC_FREE_BLKP(heap_listp); GET(HDRP(bp)) != end_status; bp = SUCC_FREE_BLKP(bp)) {
        if (bp != SUCC_FREE_BLKP(PRED_FREE_BLKP(bp))) {
            printf("error of free block list:\n");
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






