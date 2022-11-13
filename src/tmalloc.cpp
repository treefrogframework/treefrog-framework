// A simple memory allocator - Memory allocation 101, Non thread-safe
// Author: Arjun Sreedharan
// Modified by AOYAMA Kazuharu
//
#include "tmalloc.h"
#include <cstring>
#include <cstdio>
#include <mutex>
#include <errno.h>

struct alloc_header_t;

//static uint64_t pb_offset;

// Allocation table
struct alloc_table {
    // void *stk_head {nullptr};
    // void *stk_tail {nullptr};
    uint64_t headg {0};
    uint64_t tailg {0};

#if 0
    alloc_header_t *head() { return stk_head ? (alloc_header_t *)((caddr_t)stk_head + pb_offset) : nullptr; }
    alloc_header_t *tail() { return stk_tail ? (alloc_header_t *)((caddr_t)stk_tail + pb_offset) : nullptr; }
#else
    alloc_header_t *head() { return headg ? (alloc_header_t *)((caddr_t)this + headg) : nullptr; }
    alloc_header_t *tail() { return tailg ? (alloc_header_t *)((caddr_t)this + tailg) : nullptr; }
#endif
    void set_head(alloc_header_t *p)
    {
#if 0
        if (p) {
            stk_head = (caddr_t)p - pb_offset;
        } else {
            stk_head = nullptr;
        }
#else
        if (p) {
            headg = (uint64_t)p - (uint64_t)this;
        } else {
            headg = 0;
        }
#endif
    }

    void set_tail(alloc_header_t *p) {
#if 0
        if (p) {
            stk_tail = (caddr_t)p - pb_offset;
        } else {
            stk_tail = nullptr;
        }
#else
        if (p) {
            tailg = (uint64_t)p - (uint64_t)this;
        } else {
            tailg = 0;
        }
#endif
    }
};

// Program break header
struct program_break_header_t {
    // caddr_t start {nullptr};
    // caddr_t end {nullptr};
    // caddr_t current {nullptr};
    uint64_t startg {0};
    uint64_t endg {0};
    uint64_t currentg {0};
    uint64_t checksum {0};
    struct alloc_table at;
#if 0
    caddr_t start() { return start + pb_offset; }
    caddr_t end() { return end + pb_offset; }
    caddr_t current() { return current + pb_offset; }
#else
    caddr_t start() { return (caddr_t)this + startg; }
    caddr_t end() { return (caddr_t)this + endg; }
    caddr_t current() { return (caddr_t)this + currentg; }
#endif
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
#if 0
    pb_header->current += inc;
#else
    pb_header->currentg += inc;
#endif
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
    if (!initial && pb_header->checksum == ck && ck > 0) {
        // Already exists
//caddr_t orig_pb = pb_header->start() - sizeof(program_break_header_t);
//pb_offset = (uint64_t)addr - (uint64_t)orig_pb;
        memdump();
        printf("--------------------------- Already exists. addr:%p\n", addr);
        printf("--------------------------- start:   %p\n", pb_header->start());
        printf("--------------------------- end:     %p\n", pb_header->end());
        printf("--------------------------- current: %p\n", pb_header->current());
    } else {
        // new mmap
        memcpy(pb_header, &INIT_PB_HEADER, sizeof(program_break_header_t));
#if 0
        pb_header->start = pb_header->current = (caddr_t)addr + sizeof(program_break_header_t);
        pb_header->end = (caddr_t)pb_header->start + size - sizeof(program_break_header_t);
        pb_header->checksum = (uint64_t)pb_header->start + (uint64_t)pb_header->end;
#else
        pb_header->startg = pb_header->currentg = sizeof(program_break_header_t);
        pb_header->endg = size;
        pb_header->checksum = (uint64_t)size * (uint64_t)size;
#endif
        memdump();
        printf("--------------------------- addr:    %p\n", addr);
        printf("--------------------------- start:   %p\n", pb_header->start());
        printf("--------------------------- end:     %p\n", pb_header->end());
        printf("--------------------------- current: %p\n", pb_header->current());
    }
    return pb_header->start();
}


//
//--- Allocator
//
constexpr ushort CHECKDIGITS = 0x8CEF;


struct alloc_header_t {
    ushort rsv {CHECKDIGITS};
    ushort freed {0};
    uint size {0};
    // void *next {nullptr};
    // void *prev {nullptr};
    uint64_t nextg {0};
    uint64_t prevg {0};

#if 0
    alloc_header_t *next() { return next ? (alloc_header_t *)((caddr_t)next + pb_offset) : nullptr; }
    alloc_header_t *prev() { return prev ? (alloc_header_t *)((caddr_t)prev + pb_offset) : nullptr; }
#else
    alloc_header_t *next() { return nextg ? (alloc_header_t *)((caddr_t)this + nextg) : nullptr; }
    alloc_header_t *prev() { return prevg ? (alloc_header_t *)((caddr_t)this + prevg) : nullptr; }
#endif
    void set_next(alloc_header_t *p)
    {
#if 0
        if (p) {
            next = (caddr_t)p - pb_offset;
        } else {
            next = nullptr;
        }
#else
        if (p) {
            nextg = (uint64_t)p - (uint64_t)this;
        } else {
            nextg = 0;
        }
#endif
    }
    void set_prev(alloc_header_t *p)
    {
#if 0
        if (p) {
            prev = (caddr_t)p - pb_offset;
        } else {
            prev = nullptr;
        }
#else
        if (p) {
            prevg = (uint64_t)p - (uint64_t)this;
        } else {
            prevg = 0;
        }
#endif
    }
};

const alloc_header_t INIT_HEADER;

inline alloc_header_t *stk_head() { return pb_header ? pb_header->at.head() : nullptr; }
inline alloc_header_t *stk_tail() { return pb_header ? pb_header->at.tail() : nullptr; }


static alloc_header_t *free_block(uint size)
{
    alloc_header_t *p = nullptr;
    alloc_header_t *cur = stk_head();

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
        return 0;
    }

    alloc_header_t *header = (alloc_header_t*)ptr - 1;

    // checks ptr
    if (header->rsv != CHECKDIGITS) {
        return 0;
    }
    return header->size;
}

// Frees the memory space
void Tf::tfree(void *ptr)
{
    if (!pb_header) {
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

        if (header != stk_tail()) {
            break;
        }

        // header of last block
        alloc_header_t *prev = header->prev();
        pb_header->at.set_tail(prev);

        if (prev) {
            prev->set_next(nullptr);
        } else {
            //stk_head = nullptr;
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
void *Tf::tmalloc(uint size)
{
    if (!pb_header || !size) {
        return nullptr;
    }
qDebug() << "======1";
    /* Rounds up to 16bytes */
    uint d = size % 16;
    size += d ? 16 - d : 0;

    alloc_header_t *header = free_block(size);
qDebug() << "======2";
    if (header) {
qDebug() << "======3";
        /* Woah, found a free block to accomodate requested memory. */
        header->freed = 0;
        return (void *)(header + 1);
    }

    /* We need to get memory to fit in the requested block and header from OS. */
    void *block = sbrk(sizeof(alloc_header_t) + size);

    if (!block) {
qDebug() << "======4";
        return nullptr;
    }
qDebug() << "======5";

    header = (alloc_header_t *)block;
    memcpy(header, &INIT_HEADER, sizeof(INIT_HEADER));
    header->size = size;
    header->set_prev(stk_tail());

qDebug() << "======6";
    if (!stk_head()) {  // stack empty
qDebug() << "======7";
        //stk_head = header;
        pb_header->at.set_head(header);
    }

qDebug() << "======8";
    if (stk_tail()) {
qDebug() << "======9";
        stk_tail()->set_next(header);
    }
    //stk_tail = header;
qDebug() << "======10  " << header << (header + 1);
    pb_header->at.set_tail(header);
    return (void *)(header + 1);
}

// Allocates memory for an array of 'num' elements of 'size' bytes
// each and returns a pointer to the allocated memory
void *Tf::tcalloc(uint num, uint nsize)
{
    if (!num || !nsize) {
        return nullptr;
    }

    uint size = num * nsize;
    /* check mul overflow */
    if (nsize != size / num) {
        return nullptr;
    }

    void *ptr = tmalloc(size);
    if (!ptr) {
        return nullptr;
    }
qDebug() << "======11";

    memset(ptr, 0, size);  // zero clear
qDebug() << "======12";
    return ptr;
}

// Changes the size of the memory block pointed to by 'ptr' to 'size' bytes.
void *Tf::trealloc(void *ptr, uint size)
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

    void *ret = tmalloc(size);
    if (ret) {
        /* Relocate contents to the new bigger block */
        memcpy(ret, ptr, header->size);
        /* Free the old memory block */
        tfree(ptr);
    }
    return ret;
}

// Debug function to print the entire link list
void Tf::memdump()
{
    alloc_header_t *cur = stk_head();

    std::printf("-- memory block infomarion --\n");
    while (cur) {
        std::printf("addr = %p, size = %u, freed=%u, next=%p, prev=%p\n",
            (void *)cur, cur->size, cur->freed, cur->next(), cur->prev());
        cur = cur->next();
    }
    std::printf("head = %p, tail = %p, blocks = %d\n", (void *)stk_head(), (void *)stk_tail, Tf::nblocks());
}


int Tf::nblocks()
{
    int counter = 0;
    alloc_header_t *cur = stk_head();

    while (cur) {
        counter++;
        cur = cur->next();
    }
    return counter;
}
