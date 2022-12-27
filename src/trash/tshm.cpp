
#include "tshm.h"
#include "tfcore.h"
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


namespace Tf {

// Creates a new or opens an existing POSIX shared memory object
// Return: pointer
//   created:  true:new object created  false:existing object opend
void *shmcreate(const char *name, size_t size, bool *created)
{
    struct stat st;
    void *shmp;

    if (created) {
        *created = false;
    }

    int fd = shm_open(name, O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        if (errno != ENOENT) {
            // error
            goto error;
        }

        // Creates shared memory
        fd = shm_open(name, O_CREAT | O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            // error
            goto error;
        }

        if (created) {
            *created = true;
        }
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

    return shmp;

error:
    if (fd > 0) {
        tf_close(fd);
    }
    return nullptr;
}


void shmdelete(const char *name)
{
    shm_unlink(name);
}

}
