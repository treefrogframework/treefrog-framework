
#pragma once
#include "tshm.h"
#include "tfcore.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


namespace Tf {

// Creates a new or opens an existing POSIX shared memory object
// Return: pointer
//   created:  true:new object created  false:existing object opend
inline void *shmcreate(const char *path, size_t size, bool *created = nullptr)
{
    bool newmap = false;
    struct stat st;
    void *shmp;

    int fd = shm_open(path, O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        if (errno != ENOENT) {
            // error
            goto error;
        }

        // Creates shared memory
        fd = shm_open(path, O_CREAT | O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            // error
            goto error;
        }

        newmap = true;
    }

    if (fstat(fd, &st) < 0) {
        // error
        goto error;
    }

    if ((size_t)st.st_size < size) {
        if (ftruncate(fd, size) < 0) {
            // error
            goto error;
        }
    }

    shmp = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shmp == MAP_FAILED) {
        // error
        goto error;
    }

    if (created) {
        *created = newmap;
    }
    return shmp;

error:
    if (fd > 0) {
        tf_close(fd);
    }
    return nullptr;
}

}
