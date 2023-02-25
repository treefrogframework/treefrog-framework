#pragma once
#include <TGlobal>

class TSharedMemory;

namespace Tf {
struct program_break_header_t;
struct alloc_header_t;
}


class T_CORE_EXPORT TSharedMemoryAllocator {
public:
    virtual ~TSharedMemoryAllocator();

    void *malloc(uint size);
    void *calloc(uint num, uint nsize);
    void *realloc(void *ptr, uint size);
    void free(void *ptr);
    uint allocSize(const void *ptr) const;
    size_t mapSize() const;
    void *origin() const { return (void *)_origin; }
    bool lockForRead();
    bool lockForWrite();
    bool unlock();

    // Internal use
    void summary() const;
    void dump() const;  // For debug
    int countBlocks() const;  // Counts blocks
    int countFreeBlocks() const; // Counts free blocks
    size_t sizeOfFreeBlocks() const; // Total size of free blocks
    size_t dataSegmentSize() const;

    static TSharedMemoryAllocator *initialize(const QString &name, size_t size);
    static TSharedMemoryAllocator *attach(const QString &name);
    static void unlink(const QString &name);

private:
    TSharedMemoryAllocator(const QString &name);  // constructor
    char *sbrk(int64_t inc);
    void setbrk(bool initial = false);
    Tf::alloc_header_t *free_block(uint size);

    static Tf::alloc_header_t *merge(Tf::alloc_header_t *block, Tf::alloc_header_t *next);
    static Tf::alloc_header_t *divide(Tf::alloc_header_t *block, uint size);

    TSharedMemory *_sharedMemory {nullptr};
    char *_origin {nullptr};
    Tf::program_break_header_t *pb_header {nullptr};

    T_DISABLE_COPY(TSharedMemoryAllocator)
    T_DISABLE_MOVE(TSharedMemoryAllocator)
};
