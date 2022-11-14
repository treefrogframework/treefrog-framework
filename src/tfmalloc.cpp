// A simple memory allocator - Memory allocation 101, Non thread-safe
// Author: Arjun Sreedharan
// Modified by AOYAMA Kazuharu
//
#include "tfmalloc.h"
#include <cstring>
#include <cstdio>
#include <errno.h>


// Allocation table
struct alloc_table {
    uint64_t headg {0};
    uint64_t tailg {0};

    Tf::alloc_header_t *head() const { return headg ? (Tf::alloc_header_t *)((caddr_t)this + headg) : nullptr; }
    Tf::alloc_header_t *tail() const { return tailg ? (Tf::alloc_header_t *)((caddr_t)this + tailg) : nullptr; }
    void set_head(Tf::alloc_header_t *p) { headg = p ? (uint64_t)p - (uint64_t)this : 0; }
    void set_tail(Tf::alloc_header_t *p) { tailg = p ? (uint64_t)p - (uint64_t)this : 0; }
};

// Program break header
struct program_break_header_t {
    uint64_t startg {0};
    uint64_t endg {0};
    uint64_t currentg {0};
    uint64_t checksum {0};
    alloc_table at;

    caddr_t start() { return (caddr_t)this + startg; }
    caddr_t end() { return (caddr_t)this + endg; }
    caddr_t current() { return (caddr_t)this + currentg; }
    Tf::alloc_header_t *alloc_head() { return at.head(); }
    Tf::alloc_header_t *alloc_tail() { return at.tail(); }
};

const program_break_header_t INIT_PB_HEADER;
static program_break_header_t *pb_header;


// Changed the location of the program break
static caddr_t sbrk(int64_t inc)
{
    if (!pb_header) {
        errno = ENOMEM;
        return nullptr;
    }

    if (!pb_header->current() || (inc > 0 && pb_header->current() + inc > pb_header->end())
        || (inc < 0 && pb_header->current() + inc < pb_header->start())) {
        errno = ENOMEM;
        return nullptr;
    }

    caddr_t prev_break = pb_header->current();
    pb_header->currentg += inc;
    return prev_break;
}


// Sets memory space
// Return: the pointer of data area
void *Tf::setbrk(void *addr, uint size, bool initial)
{
    if (pb_header) {
        return nullptr;
    }

    pb_header = (program_break_header_t *)addr;
    printf("addr = %p\n", addr);
    printf("checksum = %ld\n", pb_header->checksum);

    // Checks checksum
    uint64_t ck = (uint64_t)size * (uint64_t)size;
    if (initial || pb_header->checksum != ck || !ck) {
        // new mmap
        memcpy(pb_header, &INIT_PB_HEADER, sizeof(program_break_header_t));
        pb_header->startg = pb_header->currentg = sizeof(program_break_header_t);
        pb_header->endg = size;
        pb_header->checksum = (uint64_t)size * (uint64_t)size;
    }

    memdump();
    printf("--------------------------- addr:    %p\n", addr);
    printf("--------------------------- start:   %p\n", pb_header->start());
    printf("--------------------------- end:     %p\n", pb_header->end());
    printf("--------------------------- current: %p\n", pb_header->current());

    return pb_header->start();
}


constexpr ushort CHECKDIGITS = 0x08C0;
const Tf::alloc_header_t INIT_HEADER { .rsv = CHECKDIGITS, .freed = false, .padding = 0 };


static Tf::alloc_header_t *free_block(uint size)
{
    if (!pb_header) {
        Q_ASSERT(0);
        return nullptr;
    }

    Tf::alloc_header_t *p = nullptr;
    Tf::alloc_header_t *cur = pb_header->alloc_head();

    while (cur) {
        /* see if there's a free block that can accomodate requested size */
        if (cur->freed && cur->size >= size) {
            if (cur->size * 0.8 <= size) {
                return cur;
            }

            if (!p || cur->size < p->size) {
                p = cur;
            }
        }
        cur = cur->next();
    }
    return p;
}


uint Tf::allocsize(const void *ptr)
{
    if (!pb_header || !ptr) {
        return 0;
    }

    if (ptr < pb_header->start() || ptr >= pb_header->end()) {
        Q_ASSERT(0);
        return 0;
    }

    alloc_header_t *header = (alloc_header_t*)ptr - 1;

    // checks ptr
    if (header->rsv != CHECKDIGITS) {
        Q_ASSERT(0);
        return 0;
    }
    return header->size;
}

// Frees the memory space
void Tf::sfree(void *ptr)
{
    if (!pb_header || !ptr || ptr == (void *)-1) {
        return;
    }

    while (ptr) {
        if (ptr < pb_header->start() || ptr >= pb_header->end()) {
            errno = ENOMEM;
            Q_ASSERT(0);
            return;
        }

        alloc_header_t *header = (alloc_header_t*)ptr - 1;

        // checks ptr
        if (header->rsv != CHECKDIGITS) {
            errno = ENOMEM;
            Q_ASSERT(0);
            return;
        }

        /*
        Check if the ptr to be freed is the last one in the
        linked list. If it is, then we could shrink the size of the
        heap and release memory to OS. Else, we will keep the ptr
        but mark it as free.
        */
        header->freed = 1;

        if (header != pb_header->alloc_tail()) {
            break;
        }

        // header of last block
        alloc_header_t *prev = header->prev();
        pb_header->at.set_tail(prev);

        if (prev) {
            prev->set_next(nullptr);
        } else {
            pb_header->at.set_head(nullptr);
        }

        /*
        sbrk() with a negative argument decrements the program break.
        So memory is released by the program to OS.
        */
        sbrk(-(header->size) - sizeof(alloc_header_t));

        /* Frees recursively */
        if (!prev || !prev->freed) {
            break;
        }
        ptr = prev + 1;
    }
}

// Allocates size bytes and returns a pointer to the allocated memory
void *Tf::smalloc(uint size)
{
    if (!pb_header || !size) {
        return nullptr;
    }
    /* Rounds up to 16bytes */
    uint d = size % 16;
    size += d ? 16 - d : 0;

    alloc_header_t *header = free_block(size);
    if (header) {
        /* Woah, found a free block to accomodate requested memory. */
        header->freed = 0;
        return (void *)(header + 1);
    }

    /* We need to get memory to fit in the requested block and header from OS. */
    void *block = sbrk(sizeof(alloc_header_t) + size);

    if (!block) {
        return nullptr;
    }

    header = (alloc_header_t *)block;
    memcpy(header, &INIT_HEADER, sizeof(INIT_HEADER));
    header->size = size;
    header->set_prev(pb_header->alloc_tail());

    if (!pb_header->alloc_head()) {  // stack empty
        pb_header->at.set_head(header);
    }

    if (pb_header->alloc_tail()) {
        pb_header->alloc_tail()->set_next(header);
    }

    pb_header->at.set_tail(header);
    return (void *)(header + 1);
}

// Allocates memory for an array of 'num' elements of 'size' bytes
// each and returns a pointer to the allocated memory
void *Tf::scalloc(uint num, uint nsize)
{
    if (!num || !nsize) {
        return nullptr;
    }

    uint size = num * nsize;
    /* check mul overflow */
    if (nsize != size / num) {
        return nullptr;
    }

    void *ptr = smalloc(size);
    if (!ptr) {
        return nullptr;
    }

    memset(ptr, 0, size);  // zero clear
    return ptr;
}

// Changes the size of the memory block pointed to by 'ptr' to 'size' bytes.
void *Tf::srealloc(void *ptr, uint size)
{
    if (!ptr || !size) {
        return nullptr;
    }

    alloc_header_t *header = (alloc_header_t*)ptr - 1;

    // checks ptr
    if (header->rsv != CHECKDIGITS) {
        errno = ENOMEM;
        return nullptr;
    }

    if (header->size >= size) {
        return ptr;
    }

    void *ret = smalloc(size);
    if (ret) {
        /* Relocate contents to the new bigger block */
        memcpy(ret, ptr, header->size);
        /* Free the old memory block */
        sfree(ptr);
    }
    return ret;
}

// Debug function to print the entire link list
void Tf::memdump()
{
    if (!pb_header) {
        Q_ASSERT(0);
        return;
    }

    alloc_header_t *cur = pb_header->alloc_head();
    int freeblk = 0;

    std::printf("-- memory block information --\n");
    while (cur) {
        std::printf("addr = %p, size = %u, freed=%u, next=%p, prev=%p\n",
            (void *)cur, cur->size, cur->freed, cur->next(), cur->prev());
        if (cur->freed) {
            freeblk++;
        }
        cur = cur->next();
    }
    std::printf("head = %p, tail = %p, blocks = %d, free = %d\n", (void *)pb_header->alloc_head(), (void *)pb_header->alloc_tail(), Tf::nblocks(), freeblk);
}


int Tf::nblocks()
{
    if (!pb_header) {
        Q_ASSERT(0);
        return 0;
    }

    int counter = 0;
    alloc_header_t *cur = pb_header->alloc_head();

    while (cur) {
        counter++;
        cur = cur->next();
    }
    return counter;
}
