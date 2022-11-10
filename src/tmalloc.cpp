// A simple memory allocator - Memory allocation 101, Non thread-safe
// Author: Arjun Sreedharan
// Modified by AOYAMA Kazuharu
//
#include "tmalloc.h"
#include <cstring>
#include <cstdio>
#include <mutex>
#include <errno.h>

struct header_t;


static uint64_t pb_offset;

// Allocation table
struct alloc_table {
    void *stk_head {nullptr};
    void *stk_tail {nullptr};

    header_t *head() { return stk_head ? (header_t *)((caddr_t)stk_head + pb_offset) : nullptr; }
    header_t *tail() { return stk_tail ? (header_t *)((caddr_t)stk_tail + pb_offset) : nullptr; }
    void set_head(header_t *p)
    {
        if (p) {
            stk_head = (caddr_t)p - pb_offset;
        } else {
            stk_head = nullptr;
        }
    }
    void set_tail(header_t *p) {
        if (p) {
            stk_tail = (caddr_t)p - pb_offset;
        } else {
            stk_tail = nullptr;
        }
    }
};

// Program break header
struct program_break_header_t {
    caddr_t start {nullptr};
    caddr_t end {nullptr};
    caddr_t current {nullptr};
    uint64_t checksum {0};
    struct alloc_table at;

    caddr_t start_ptr() { return start + pb_offset; }
    caddr_t end_ptr() { return end + pb_offset; }
    caddr_t current_ptr() { return current + pb_offset; }
};

const program_break_header_t INIT_PB_HEADER;
static program_break_header_t *pb_header;

// Initiaize memory space
void Tf::initbrk(void *addr, uint size)
{
    if (pb_header) {
        return;
    }

    pb_header = (program_break_header_t *)addr;

    printf("addr = %p\n", addr);
    printf("checksum = %ld\n", pb_header->checksum);

    // Checks checksum
    uint64_t ck = (uint64_t)pb_header->start_ptr() + (uint64_t)pb_header->end_ptr();
	if (pb_header->checksum == ck && ck > 0) {
        // Already exists
        caddr_t orig_pb = pb_header->start_ptr() - sizeof(program_break_header_t);
        pb_offset = (uint64_t)addr - (uint64_t)orig_pb;
        memdump();
        // printf("--------------------------- Already exists. this addr:%ld  offset:%ld\n", addr, pb_offset);
        // printf("--------------------------- start:   %ld\n", pb_header->start_ptr());
        // printf("--------------------------- end:     %ld\n", pb_header->end_ptr());
        // printf("--------------------------- current: %ld\n", pb_header->current_ptr());
    } else {
        // new mmap
        memcpy(addr, &INIT_PB_HEADER, sizeof(program_break_header_t));
        pb_header->start = pb_header->current = (caddr_t)addr + sizeof(program_break_header_t);
        pb_header->end = (caddr_t)pb_header->start + size - sizeof(program_break_header_t);
        pb_header->checksum = (uint64_t)pb_header->start + (uint64_t)pb_header->end;
        // printf("---------------------==--- start:   %ld\n", pb_header->start_ptr());
        // printf("---------------------==--- end:     %ld\n", pb_header->end_ptr());
        // printf("---------------------==--- current: %ld\n", pb_header->current_ptr());
    }
}

// Changed the location of the program break
static caddr_t sbrk(int inc)
{
    if (!pb_header) {
        errno = ENOMEM;
        return nullptr;
    }

    if (!pb_header->current_ptr() || (inc > 0 && pb_header->current_ptr() + inc > pb_header->end_ptr())
    || (inc < 0 && pb_header->current_ptr() + inc < pb_header->start_ptr())) {
        errno = ENOMEM;
        return nullptr;
    }

    caddr_t prev_break = pb_header->current_ptr();
    pb_header->current += inc;
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
// static header_t *stk_head = nullptr;
// static header_t *stk_tail = nullptr;
inline header_t *stk_head() { return pb_header->at.head(); }
inline header_t *stk_tail() { return pb_header->at.tail(); }

static header_t *free_block(uint size)
{
    header_t *p = nullptr;
	header_t *cur = stk_head();

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
        if (ptr < pb_header->start_ptr() || ptr >= pb_header->end_ptr()) {
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

        if (header != stk_tail()) {
            break;
        }

        // header of last block
        header_t *prev = header->prev;
        pb_header->at.set_tail(prev);

        if (prev) {
            prev->next = nullptr;
        } else {
            //stk_head = nullptr;
            pb_header->at.set_head(nullptr);
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
	header->prev = stk_tail();

	if (!stk_head()) {  // stack empty
		//stk_head = header;
        pb_header->at.set_head(header);
	}

	if (stk_tail()) {
		stk_tail()->next = header;
	}
	//stk_tail = header;
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
	header_t *cur = stk_head();

    std::printf("-- memory block infomarion --\n");
	while (cur) {
		std::printf("addr = %p, size = %u, freed=%u, next=%p, prev=%p\n",
			(void *)cur, cur->size, cur->freed, cur->next, cur->prev);
		cur = cur->next;
	}
	std::printf("head = %p, tail = %p, blocks = %d\n", (void *)stk_head(), (void *)stk_tail, Tf::nblocks());
}


int Tf::nblocks()
{
    int counter = 0;
    header_t *cur = stk_head();

	while (cur) {
        counter++;
		cur = cur->next;
    }
    return counter;
}
