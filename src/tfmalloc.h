#pragma once
#include <TGlobal>



namespace Tf {
struct program_break_header_t;
struct alloc_header_t;
}


class T_CORE_EXPORT TSharedMemoryAllocator {
public:
    TSharedMemoryAllocator(const QString &name, size_t size);

    void *malloc(uint size);
    void *calloc(uint num, uint nsize);
    void *realloc(void *ptr, uint size);
    void free(void *ptr);
    uint allocSize(const void *ptr);
    size_t mapSize() const { return _size; }
    bool isNew() const { return _newmap; }
    void *origin() const { return (void *)_origin; }

    // Internal use
    void summary();
    void dump();  // For debug
    int nblocks();  // Counts blocks

private:
    caddr_t sbrk(int64_t inc);
    void *setbrk(void *start, uint size, bool initial = false);
    Tf::alloc_header_t *free_block(uint size);

    QString _name;
    size_t _size {0};
    void *_shm {nullptr};
    bool _newmap {false};
    caddr_t _origin {nullptr};
    Tf::program_break_header_t *pb_header {nullptr};

    T_DISABLE_COPY(TSharedMemoryAllocator)
    T_DISABLE_MOVE(TSharedMemoryAllocator)
};
