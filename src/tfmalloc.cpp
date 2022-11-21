// A simple memory allocator - Memory allocation 101, Non thread-safe
// Author: Arjun Sreedharan
// Modified by AOYAMA Kazuharu
//
#include "tfmalloc.h"
#include "tshm.h"
#include "tsystemglobal.h"
#include <cstring>
#include <cstdio>
#include <pthread.h>
#include <errno.h>

namespace Tf {

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
    pthread_mutex_t mutex;
    alloc_table at;

    caddr_t start() { return (caddr_t)this + startg; }
    caddr_t end() { return (caddr_t)this + endg; }
    caddr_t current() { return (caddr_t)this + currentg; }
    Tf::alloc_header_t *alloc_head() { return at.head(); }
    Tf::alloc_header_t *alloc_tail() { return at.tail(); }
};

struct alloc_header_t {
    ushort rsv;
    bool freed : 1;
    ushort padding : 15;
    uint size {0};
    uint64_t nextg {0};
    uint64_t prevg {0};

    alloc_header_t *next() const { return nextg ? (alloc_header_t *)((caddr_t)this + nextg) : nullptr; }
    alloc_header_t *prev() const { return prevg ? (alloc_header_t *)((caddr_t)this + prevg) : nullptr; }
    void set_next(alloc_header_t *p) { nextg = p ? (uint64_t)p - (uint64_t)this : 0; }
    void set_prev(alloc_header_t *p) { prevg = p ? (uint64_t)p - (uint64_t)this : 0; }
};

}


class ShmLocker {
public:
    explicit ShmLocker(Tf::program_break_header_t *header) : _header(header)
    {
        if (_header) {
            tf_pthread_mutex_lock(&_header->mutex);
        }
    }
    ~ShmLocker()
    {
        if (_header) {
            tf_pthread_mutex_unlock(&_header->mutex);
        }
    }
private:
    Tf::program_break_header_t *_header {nullptr};
};


TSharedMemoryAllocator::TSharedMemoryAllocator(const QString &name, size_t size) :
    _name(name), _size(size)
{
    if (_name.isEmpty()) {
        _shm = new char[size];
    } else {
        _shm = Tf::shmcreate(qUtf8Printable(name), _size, &_newmap);
    }
    _origin = (caddr_t)setbrk(_shm, size, _newmap);
}


TSharedMemoryAllocator::~TSharedMemoryAllocator()
{
    ShmLocker locker(pb_header);

    if (_name.isEmpty()) {
        delete (char*)_shm;
    }
}


// Changed the location of the program break
caddr_t TSharedMemoryAllocator::sbrk(int64_t inc)
{
    if (!pb_header) {
        errno = ENOMEM;
        return nullptr;
    }

    ShmLocker locker(pb_header);

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
// Return: the origin pointer of data area
void *TSharedMemoryAllocator::setbrk(void *addr, uint size, bool initial)
{
    static const Tf::program_break_header_t INIT_PB_HEADER = []() {
        Tf::program_break_header_t header;
        pthread_mutexattr_t mat;

        pthread_mutexattr_init(&mat);
        pthread_mutexattr_settype(&mat, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutexattr_setpshared(&mat, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&header.mutex, &mat);
        return header;
    }();

    if (pb_header) {
        return nullptr;
    }

    pb_header = (Tf::program_break_header_t *)addr;
    tSystemDebug("addr = %p\n", addr);
    tSystemDebug("checksum = %ld\n", pb_header->checksum);

    // Checks checksum
    uint64_t ck = (uint64_t)size * (uint64_t)size;
    if (initial || pb_header->checksum != ck || !ck) {
        // new mmap
        memcpy(pb_header, &INIT_PB_HEADER, sizeof(Tf::program_break_header_t));
        pb_header->startg = pb_header->currentg = sizeof(Tf::program_break_header_t);
        pb_header->endg = size;
        pb_header->checksum = (uint64_t)size * (uint64_t)size;
    }

    //memdump();
    return pb_header->start() + sizeof(Tf::alloc_header_t);
}


constexpr ushort CHECKDIGITS = 0x08C0;
const Tf::alloc_header_t INIT_HEADER { .rsv = CHECKDIGITS, .freed = false, .padding = 0 };


Tf::alloc_header_t *TSharedMemoryAllocator::free_block(uint size)
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


uint TSharedMemoryAllocator::allocSize(const void *ptr)
{
    if (!pb_header || !ptr) {
        return 0;
    }

    ShmLocker locker(pb_header);

    if (ptr < pb_header->start() || ptr >= pb_header->end()) {
        Q_ASSERT(0);
        return 0;
    }

    Tf::alloc_header_t *header = (Tf::alloc_header_t*)ptr - 1;

    // checks ptr
    if (header->rsv != CHECKDIGITS) {
        Q_ASSERT(0);
        return 0;
    }
    return header->size;
}

// Frees the memory space
void TSharedMemoryAllocator::free(void *ptr)
{
    if (!pb_header || !ptr || ptr == (void *)-1) {
        return;
    }

    ShmLocker locker(pb_header);

    while (ptr) {
        if (ptr < pb_header->start() || ptr >= pb_header->end()) {
            errno = ENOMEM;
            Q_ASSERT(0);
            return;
        }

        Tf::alloc_header_t *header = (Tf::alloc_header_t*)ptr - 1;

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
        Tf::alloc_header_t *prev = header->prev();
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
        TSharedMemoryAllocator::sbrk(-(header->size) - sizeof(Tf::alloc_header_t));

        /* Frees recursively */
        if (!prev || !prev->freed) {
            break;
        }
        ptr = prev + 1;
    }
}

// Allocates size bytes and returns a pointer to the allocated memory
void *TSharedMemoryAllocator::malloc(uint size)
{
    if (!pb_header || !size) {
        return nullptr;
    }

    ShmLocker locker(pb_header);

    /* Rounds up to 16bytes */
    uint d = size % 16;
    size += d ? 16 - d : 0;

    Tf::alloc_header_t *header = free_block(size);
    if (header) {
        /* Woah, found a free block to accomodate requested memory. */
        header->freed = 0;
        return (void *)(header + 1);
    }

    /* We need to get memory to fit in the requested block and header from OS. */
    void *block = TSharedMemoryAllocator::sbrk(sizeof(Tf::alloc_header_t) + size);

    if (!block) {
        return nullptr;
    }

    header = (Tf::alloc_header_t *)block;
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
void *TSharedMemoryAllocator::calloc(uint num, uint nsize)
{
    if (!num || !nsize) {
        return nullptr;
    }

    ShmLocker locker(pb_header);

    uint size = num * nsize;
    /* check mul overflow */
    if (nsize != size / num) {
        return nullptr;
    }

    void *ptr = TSharedMemoryAllocator::malloc(size);
    if (!ptr) {
        return nullptr;
    }

    memset(ptr, 0, size);  // zero clear
    return ptr;
}

// Changes the size of the memory block pointed to by 'ptr' to 'size' bytes.
void *TSharedMemoryAllocator::realloc(void *ptr, uint size)
{
    if (!ptr || !size) {
        return nullptr;
    }

    ShmLocker locker(pb_header);

    Tf::alloc_header_t *header = (Tf::alloc_header_t*)ptr - 1;

    // checks ptr
    if (header->rsv != CHECKDIGITS) {
        errno = ENOMEM;
        return nullptr;
    }

    if (header->size >= size) {
        return ptr;
    }

    void *ret = TSharedMemoryAllocator::malloc(size);
    if (ret) {
        /* Relocate contents to the new bigger block */
        memcpy(ret, ptr, header->size);
        /* Free the old memory block */
        TSharedMemoryAllocator::free(ptr);
    }
    return ret;
}

// Prints summary
void TSharedMemoryAllocator::summary()
{
    if (!pb_header) {
        Q_ASSERT(0);
        return;
    }

    ShmLocker locker(pb_header);

    Tf::alloc_header_t *cur = pb_header->alloc_head();
    int freeblk = 0;
    int used = 0;

    tSystemDebug("-- memory block summary --\n");
    while (cur) {
        if (cur->freed) {
            freeblk++;
        } else {
            used += cur->size;
        }
        cur = cur->next();
    }
    tSystemDebug("blocks = %d, free = %d, used = %d\n", nblocks(), freeblk, used);
}

// Debug function to print the entire link list
void TSharedMemoryAllocator::dump()
{
    if (!pb_header) {
        Q_ASSERT(0);
        return;
    }

    ShmLocker locker(pb_header);

    Tf::alloc_header_t *cur = pb_header->alloc_head();
    int freeblk = 0;
    int used = 0;

    tSystemDebug("-- memory block information --\n");
    while (cur) {
        tSystemDebug("addr = %p, size = %u, freed=%u, next=%p, prev=%p\n",
            (void *)cur, cur->size, cur->freed, cur->next(), cur->prev());

        if (cur->freed) {
            freeblk++;
        } else {
            used += cur->size;
        }
        cur = cur->next();
    }
    tSystemDebug("head = %p, tail = %p, blocks = %d, free = %d, used = %d\n", pb_header->alloc_head(),
        pb_header->alloc_tail(), nblocks(), freeblk, used);
}


int TSharedMemoryAllocator::nblocks()
{
    if (!pb_header) {
        Q_ASSERT(0);
        return 0;
    }

    ShmLocker locker(pb_header);

    int counter = 0;
    Tf::alloc_header_t *cur = pb_header->alloc_head();

    while (cur) {
        counter++;
        cur = cur->next();
    }
    return counter;
}

// Locks recursively
void TSharedMemoryAllocator::lock()
{
    if (pb_header) {
        tf_pthread_mutex_lock(&pb_header->mutex);
    }
}

// Unlocks
void TSharedMemoryAllocator::unlock()
{
    if (pb_header) {
        tf_pthread_mutex_unlock(&pb_header->mutex);
    }
}
