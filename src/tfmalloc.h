#pragma once
#include <cstdlib>

namespace Tf {

void *setbrk(void *start, uint size, bool initial = false);
void *smalloc(uint size);
void *scalloc(uint num, uint nsize);
void *srealloc(void *ptr, uint size);
void sfree(void *ptr);
uint allocsize(const void *ptr);

// Internal use
void shmsummary();
void shmdump();  // For debug
int shmblocks();  // Counts blocks


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
