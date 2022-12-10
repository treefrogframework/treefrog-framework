#include "tsharedmemory.h"
#include "tfcore.h"
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


TSharedMemory::TSharedMemory(const QString &name) :
    _name(name)
{ }


TSharedMemory::~TSharedMemory()
{
    detach();

    if (_fd > 0) {
        tf_close(_fd);
        _fd = 0;
    }
}


bool TSharedMemory::create(size_t size)
{
    if (_data || size == 0) {
        return false;
    }

    struct stat st;
    void *shmp;

    // Creates shared memory
    _fd = shm_open(qUtf8Printable(_name), O_CREAT | O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if (_fd < 0) {
        // error
        goto error;
    }

    if (fstat(_fd, &st) < 0) {
        // error
        goto error;
    }

    if ((size_t)st.st_size < size) {
        if (ftruncate(_fd, size) < 0) {
            // error
            goto error;
        }
    }

    _data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
    if (!_data || _data == MAP_FAILED) {
        // error
        goto error;
    }

    _size = size;
    return true;

error:
    if (_fd > 0) {
        tf_close(_fd);
        _fd = 0;
    }

    _data = nullptr;
    _size = 0;
    return false;
}


void TSharedMemory::unlink()
{
    shm_unlink(qUtf8Printable(_name));
}


bool TSharedMemory::attach()
{
    struct stat st;

    _fd = shm_open(qUtf8Printable(_name), O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if (_fd < 0) {
        if (errno != ENOENT) {
            // error
            goto error;
        }
    }

    if (fstat(_fd, &st) < 0) {
        // error
        goto error;
    }

    _data = mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
    if (_data == MAP_FAILED) {
        // error
        goto error;
    }

    _size = st.st_size;
    return true;

error:
    if (_fd > 0) {
        tf_close(_fd);
        _fd = 0;
    }

    _data = nullptr;
    _size = 0;
    return false;
}


bool TSharedMemory::detach()
{
    if (!_data) {
        return false;
    }

    munmap(_data, _size);

    if (_fd > 0) {
        tf_close(_fd);
        _fd = 0;
    }

    _data = nullptr;
    _size = 0;
    return true;
}


bool TSharedMemory::lock()
{

}


bool TSharedMemory::unlock()
{

}

