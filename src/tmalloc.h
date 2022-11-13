#pragma once
#include <cstdlib>

namespace Tf {

void *setbrk(void *start, uint size, bool initial = false);
void *tmalloc(uint size);
void *tcalloc(uint num, uint nsize);
void *trealloc(void *ptr, uint size);
void tfree(void *ptr);
uint allocsize(const void *ptr);

// Internal use
void memdump();  // For debug
int nblocks();  // Counts blocks

}
