/* Copyright (c) 2022, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsharedmemoryallocator.h"
#include "tsharedmemory.h"
#include "tsystemglobal.h"
#include <cstring>
#include <cerrno>

constexpr ushort CHECKDIGITS = 0x08C0;

namespace Tf {

// Allocation table
struct alloc_table {
    uintptr_t headg {0};
    uintptr_t tailg {0};
    uint64_t used {0};  // used bytes

    Tf::alloc_header_t *head() const { return headg ? (Tf::alloc_header_t *)((uintptr_t)this + headg) : nullptr; }
    Tf::alloc_header_t *tail() const { return tailg ? (Tf::alloc_header_t *)((uintptr_t)this + tailg) : nullptr; }
    void set_head(Tf::alloc_header_t *p) { headg = p ? (uintptr_t)p - (uintptr_t)this : 0; }
    void set_tail(Tf::alloc_header_t *p) { tailg = p ? (uintptr_t)p - (uintptr_t)this : 0; }
};

// Program break header
struct program_break_header_t {
    uintptr_t startg {0};
    uintptr_t endg {0};
    uintptr_t currentg {0};
    uint64_t checksum {0};
    alloc_table at;

    char *start() { return (char *)this + startg; }
    char *end() { return (char *)this + endg; }
    char *current() { return (char *)this + currentg; }
    Tf::alloc_header_t *alloc_head() { return at.head(); }
    Tf::alloc_header_t *alloc_tail() { return at.tail(); }
};


struct alloc_header_t {
    ushort rsv {CHECKDIGITS};
    uchar freed {0};
    uchar padding {0};
    uint size {0};
    uintptr_t nextg {0};
    uintptr_t prevg {0};

    alloc_header_t *next() const { return nextg ? (alloc_header_t *)((char *)this + nextg) : nullptr; }
    alloc_header_t *prev() const { return prevg ? (alloc_header_t *)((char *)this + prevg) : nullptr; }
    void set_next(alloc_header_t *p) { nextg = p ? (uintptr_t)p - (uintptr_t)this : 0; }
    void set_prev(alloc_header_t *p) { prevg = p ? (uintptr_t)p - (uintptr_t)this : 0; }
};

} // namespace Tf

const Tf::alloc_header_t INIT_HEADER;


TSharedMemoryAllocator *TSharedMemoryAllocator::initialize(const QString &name, size_t size)
{
    auto *instance = new TSharedMemoryAllocator(name);

    if (instance->_sharedMemory->create(size)) {
        instance->setbrk(true);
    } else {
        delete instance;
        instance = nullptr;
    }
    return instance;
}


TSharedMemoryAllocator *TSharedMemoryAllocator::attach(const QString &name)
{
    auto *instance = new TSharedMemoryAllocator(name);

    if (instance->_sharedMemory->attach()) {
        instance->setbrk(false);
    } else {
        delete instance;
        instance = nullptr;
    }
    return instance;
}


void TSharedMemoryAllocator::unlink(const QString &name)
{
    TSharedMemory(name).unlink();
}


TSharedMemoryAllocator::TSharedMemoryAllocator(const QString &name) :
    _sharedMemory(new TSharedMemory(name))
{ }


TSharedMemoryAllocator::~TSharedMemoryAllocator()
{
    delete _sharedMemory;
}


// Changed the location of the program break
char *TSharedMemoryAllocator::sbrk(int64_t inc)
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

    char *prev_break = pb_header->current();
    pb_header->currentg += inc;
    //tSystemDebug("sbrk: inc: %ld, current: %p", inc, pb_header->current());
    return prev_break;
}


// Sets memory space
// Return: the origin pointer of data area
void TSharedMemoryAllocator::setbrk(bool initial)
{
    static const Tf::program_break_header_t INIT_PB_HEADER;

    if (pb_header) {
        return;
    }

    pb_header = (Tf::program_break_header_t *)_sharedMemory->data();
    tSystemDebug("addr = %p", _sharedMemory->data());

    // Checks checksum
    uint64_t ck = (uint64_t)_sharedMemory->size() * (uint64_t)_sharedMemory->size();
    if (initial || pb_header->checksum != ck || !ck) {
        // new mmap
        std::memcpy(pb_header, &INIT_PB_HEADER, sizeof(Tf::program_break_header_t));
        pb_header->startg = pb_header->currentg = sizeof(Tf::program_break_header_t);
        pb_header->endg = _sharedMemory->size();
        pb_header->checksum = (uint64_t)_sharedMemory->size() * (uint64_t)_sharedMemory->size();
    }
    tSystemDebug("checksum = %lu", pb_header->checksum);

    _origin = pb_header->start() + sizeof(Tf::alloc_header_t);
}


Tf::alloc_header_t *TSharedMemoryAllocator::free_block(uint size)
{
    if (!pb_header) {
        Q_ASSERT(0);
        return nullptr;
    }

    Tf::alloc_header_t *p = nullptr;
    Tf::alloc_header_t *cur = pb_header->alloc_head();

    while (cur) {
        if (cur->freed && cur->size >= size) {
            if (size >= cur->size * 0.8) {
                return cur;
            }

            if (!p || cur->size < p->size) {
                p = cur;
            }
        }
        cur = cur->next();
    }

    if (p && p->size - size > sizeof(Tf::alloc_header_t) * 10) {
        // If free space is more than 240 bytes
        divide(p, size);
    }

    return p;
}


uint TSharedMemoryAllocator::allocSize(const void *ptr) const
{
    if (!pb_header || !ptr) {
        return 0;
    }

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


Tf::alloc_header_t *TSharedMemoryAllocator::divide(Tf::alloc_header_t *block, uint size)
{
    uint d = size % 32;
    size += d ? 32 - d : 0;

    if (!block || block->size < size + sizeof(Tf::alloc_header_t)) {
        return nullptr;
    }

    auto *header = (Tf::alloc_header_t *)((char *)(block + 1) + size);
    std::memcpy(header, &INIT_HEADER, sizeof(INIT_HEADER));
    header->freed = 1;
    header->size = block->size - size - sizeof(Tf::alloc_header_t);
    header->set_prev(block);
    header->set_next(block->next());
    if (block->next()) {
        block->next()->set_prev(header);
    }

    block->set_next(header);
    block->size = size;
    return header;
}


Tf::alloc_header_t *TSharedMemoryAllocator::merge(Tf::alloc_header_t *block, Tf::alloc_header_t *next)
{
    if (!block || !next) {
        return block;
    }

    block->set_next(next->next());
    block->size += sizeof(Tf::alloc_header_t) + next->size;
    if (next->next()) {
        next->next()->set_prev(block);
    }
    return block;
}

// Frees the memory space
void TSharedMemoryAllocator::free(void *ptr)
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

        Tf::alloc_header_t *header = (Tf::alloc_header_t*)ptr - 1;

        // checks ptr
        if (header->rsv != CHECKDIGITS) {
            errno = ENOMEM;
            Q_ASSERT(0);
            return;
        }

        // marks as free
        if (!header->freed) {
            header->freed = 1;
            pb_header->at.used -= sizeof(Tf::alloc_header_t) + header->size;
        }

        if (header != pb_header->alloc_tail()) {
            // next block
            Tf::alloc_header_t *p = header->next();
            if (p->freed) {
                merge(header, p);
            }

            // prev block
            p = header->prev();
            if (p && p->freed) {
                merge(p, header);
            }

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

        // memory released
        TSharedMemoryAllocator::sbrk(0 - (int64_t)header->size - (int64_t)sizeof(Tf::alloc_header_t));

        // frees recursively
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

    // rounds up to 32bytes
    uint d = size % 32;
    size += d ? 32 - d : 0;

    Tf::alloc_header_t *header = free_block(size);
    if (header) {
        // found a free block
        header->freed = 0;
        pb_header->at.used += sizeof(Tf::alloc_header_t) + header->size;
        return (void *)(header + 1);
    }

    // get memory to fit
    void *block = TSharedMemoryAllocator::sbrk(sizeof(Tf::alloc_header_t) + size);

    if (!block) {
        return nullptr;
    }

    header = (Tf::alloc_header_t *)block;
    std::memcpy(header, &INIT_HEADER, sizeof(INIT_HEADER));
    header->size = size;
    header->set_prev(pb_header->alloc_tail());
    pb_header->at.used += sizeof(Tf::alloc_header_t) + header->size;

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

    uint size = num * nsize;
    // check mul overflow
    if (nsize != size / num) {
        return nullptr;
    }

    void *ptr = TSharedMemoryAllocator::malloc(size);
    if (!ptr) {
        return nullptr;
    }

    std::memset(ptr, 0, size);  // zero clear
    return ptr;
}

// Changes the size of the memory block pointed to by 'ptr' to 'size' bytes.
void *TSharedMemoryAllocator::realloc(void *ptr, uint size)
{
    if (!ptr || !size) {
        return nullptr;
    }

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
        // relocate contents
        std::memcpy(ret, ptr, header->size);
        // free the old block
        TSharedMemoryAllocator::free(ptr);
    }
    return ret;
}


size_t TSharedMemoryAllocator::mapSize() const
{
    return _sharedMemory->size();
}


// Prints summary
void TSharedMemoryAllocator::summary() const
{
    if (!pb_header) {
        Q_ASSERT(0);
        return;
    }

    tSystemDebug("-- memory block summary --");
    tSystemDebug("table info: blocks = %d, free = %d, used = %lu", countBlocks(), countFreeBlocks(), pb_header->at.used);
}

// Debug function to print the entire link list
void TSharedMemoryAllocator::dump() const
{
    if (!pb_header) {
        Q_ASSERT(0);
        return;
    }

    Tf::alloc_header_t *cur = pb_header->alloc_head();

    tSystemDebug("-- memory block information --");
    tSystemDebug("table info: blocks = %d, free = %d, used = %lu, free-size = %lu, segment-size = %lu", countBlocks(), countFreeBlocks(), pb_header->at.used, sizeOfFreeBlocks(), dataSegmentSize());
    tSystemDebug("block info: head = %p, tail = %p", pb_header->alloc_head(), pb_header->alloc_tail());
    while (cur) {
        tSystemDebug("addr = %p, size = %u, freed = %u, next = %p, prev = %p",
            (void *)cur, cur->size, cur->freed, cur->next(), cur->prev());
        cur = cur->next();
    }
}


int TSharedMemoryAllocator::countBlocks() const
{
    if (!pb_header) {
        Q_ASSERT(0);
        return 0;
    }

    int counter = 0;
    Tf::alloc_header_t *cur = pb_header->alloc_head();

    while (cur) {
        counter++;
        cur = cur->next();
    }
    return counter;
}


int TSharedMemoryAllocator::countFreeBlocks() const
{
    if (!pb_header) {
        Q_ASSERT(0);
        return 0;
    }

    int counter = 0;
    Tf::alloc_header_t *cur = pb_header->alloc_head();

    while (cur) {
        if (cur->freed) {
            counter++;
        }
        cur = cur->next();
    }
    return counter;
}


size_t TSharedMemoryAllocator::sizeOfFreeBlocks() const
{
    if (!pb_header) {
        Q_ASSERT(0);
        return 0;
    }

    size_t size = 0;
    Tf::alloc_header_t *cur = pb_header->alloc_head();

    while (cur) {
        if (cur->freed) {
            size += cur->size;
        }
        cur = cur->next();
    }
    return size;
}


size_t TSharedMemoryAllocator::dataSegmentSize() const
{
    if (!pb_header) {
        Q_ASSERT(0);
        return 0;
    }

    return pb_header->current() - pb_header->start();
}


bool TSharedMemoryAllocator::lockForRead()
{
    return _sharedMemory->lockForRead();
}


bool TSharedMemoryAllocator::lockForWrite()
{
    return _sharedMemory->lockForWrite();
}


bool TSharedMemoryAllocator::unlock()
{
    return _sharedMemory->unlock();
}
