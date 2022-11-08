// A simple memory allocator - Memory allocation 101, Non thread-safe
// Author: Arjun Sreedharan
// Modified by AOYAMA Kazuharu
//
#include "tmalloc.h"
#include <cstring>
#include <cstdio>
#include <errno.h>

static caddr_t program_break_start = nullptr;
static caddr_t program_break_end = nullptr;
static caddr_t current_break = nullptr;

// Initiaize memory space
void Tf::initbrk(void *addr, uint size)
{
	if (!program_break_start) {
		program_break_start = current_break = (caddr_t)addr;
		program_break_end = (caddr_t)program_break_start + size;
	}
}

// Changed the location of the program break
static caddr_t sbrk(int inc) {
  if (!current_break || (inc > 0 && current_break + inc > program_break_end)
    || (inc < 0 && current_break + inc < program_break_start)) {
    errno = ENOMEM;
    return nullptr;
  }

  caddr_t prev_break = current_break;
  current_break += inc;
  return prev_break;
}


//
//--- Allocator
//
constexpr ushort CHECKDIGITS = 0x8CEF;

struct header_t {
    ushort rsv {CHECKDIGITS};
    ushort freed {0};
    uint size {0};
    header_t *next {nullptr};
    header_t *prev {nullptr};
};

const header_t INIT_HEADER;
static header_t *stk_head = nullptr;
static header_t *stk_tail = nullptr;


static header_t *free_block(uint size)
{
    header_t *p = nullptr;
	header_t *cur = stk_head;

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
		cur = cur->next;
	}
	return p;
}

// Frees the memory space
void Tf::tfree(void *ptr)
{
    while (ptr) {
        if (ptr < program_break_start || ptr >= program_break_end) {
            errno = ENOMEM;
            return;
        }

        header_t *header = (header_t*)ptr - 1;

        // checks ptr
        if (header->rsv != CHECKDIGITS) {
            errno = ENOMEM;
            return;
        }

        /*
        Check if the ptr to be freed is the last one in the
        linked list. If it is, then we could shrink the size of the
        heap and release memory to OS. Else, we will keep the ptr
        but mark it as free.
        */
        header->freed = 1;

        if (header != stk_tail) {
            break;
        }

        // header of last block
        header_t *prev = stk_tail = header->prev;
        if (prev) {
            prev->next = nullptr;
        } else {
            stk_head = nullptr;
        }

        /*
        sbrk() with a negative argument decrements the program break.
        So memory is released by the program to OS.
        */
        sbrk(-(header->size) - sizeof(header_t));

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
	if (!size) {
		return nullptr;
	}

	/* Rounds up to 16bytes */
	uint d = size % 16;
	size += d ? 16 - d : 0;

	header_t *header = free_block(size);
	if (header) {
		/* Woah, found a free block to accomodate requested memory. */
		header->freed = 0;
		return (void *)(header + 1);
	}

	/* We need to get memory to fit in the requested block and header from OS. */
	void *block = sbrk(sizeof(header_t) + size);

	if (!block) {
		return nullptr;
	}

	header = (header_t *)block;
    memcpy(header, &INIT_HEADER, sizeof(INIT_HEADER));
	header->size = size;
	header->prev = stk_tail;

	if (!stk_head) {  // stack empty
		stk_head = header;
	}

	if (stk_tail) {
		stk_tail->next = header;
	}
	stk_tail = header;
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

	memset(ptr, 0, size);  // zero clear
	return ptr;
}

// Changes the size of the memory block pointed to by 'ptr' to 'size' bytes.
void *Tf::trealloc(void *ptr, uint size)
{
	if (!ptr || !size) {
		return nullptr;
	}

	header_t *header = (header_t*)ptr - 1;

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
	header_t *cur = stk_head;

    std::printf("-- memory block infomarion --\n");
	while (cur) {
		std::printf("addr = %p, size = %u, freed=%u, next=%p, prev=%p\n",
			(void *)cur, cur->size, cur->freed, cur->next, cur->prev);
		cur = cur->next;
	}
	std::printf("head = %p, tail = %p, blocks = %d\n", (void *)stk_head, (void *)stk_tail, Tf::nblocks());
}


int Tf::nblocks()
{
    int counter = 0;
    header_t *cur = stk_head;

	while (cur) {
        counter++;
		cur = cur->next;
    }
    return counter;
}
