#pragma once
#include <cstdlib>

namespace Tf {

void initbrk(void *start, uint size);
void *tmalloc(uint size);
void *tcalloc(uint num, uint nsize);
void *trealloc(void *block, uint size);
void tfree(void *block);

// Internal use
void memdump();  // For debug
int nblocks();  // Counts blocks

}
