/* Copyright (c) 2022, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsharedmemory.h"
#include "tfcore.h"
#include "tsystemglobal.h"
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>


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
    if (_ptr || size == 0 || _name.isEmpty()) {
        return false;
    }

    struct stat st;
    header_t *header = nullptr;

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

    size = std::max(sizeof(header_t) * 10, size);
    if ((size_t)st.st_size < size) {
        if (ftruncate(_fd, size) < 0) {
            // error
            goto error;
        }
    }

    _ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
    if (!_ptr || _ptr == MAP_FAILED) {
        // error
        goto error;
    }

    header = new (_ptr) header_t{};
    initRwlock(header);

    _size = size;
    tSystemDebug("SharedMemory created.  name:{} size:{}", _name, (qulonglong)_size);
    return true;

error:
    tSystemError("SharedMemory create error.  name:{} size:{} [{}:{}]", _name, (qulonglong)size, __FILE__, __LINE__);

    if (_fd > 0) {
        tf_close(_fd);
        _fd = 0;
    }

    _ptr = nullptr;
    _size = 0;
    return false;
}


void TSharedMemory::unlink()
{
    releaseRwlock((header_t *)_ptr);
    shm_unlink(qUtf8Printable(_name));
    tSystemDebug("SharedMemory unlinked.  name:{}", _name);
}


bool TSharedMemory::attach()
{
    if (_ptr || _name.isEmpty()) {
        return false;
    }

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

    _ptr = mmap(nullptr, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
    if (_ptr == MAP_FAILED) {
        // error
        goto error;
    }

    _size = st.st_size;
    tSystemDebug("SharedMemory attached.  name:{} size:{}", _name, (quint64)_size);
    return true;

error:
    tSystemError("SharedMemory attach error  [{}:{}]", __FILE__, __LINE__);

    if (_fd > 0) {
        tf_close(_fd);
        _fd = 0;
    }

    _ptr = nullptr;
    _size = 0;
    return false;
}


bool TSharedMemory::detach()
{
    if (!_ptr) {
        return false;
    }

    munmap(_ptr, _size);

    if (_fd > 0) {
        tf_close(_fd);
        _fd = 0;
    }

    _ptr = nullptr;
    _size = 0;
    tSystemDebug("SharedMemory detached.  name:{}", _name);
    return true;
}


void *TSharedMemory::data()
{
    return (char *)_ptr + sizeof(header_t);
}


const void *TSharedMemory::data() const
{
    return (char *)_ptr + sizeof(header_t);
}


QString TSharedMemory::name() const
{
    return _name;
}


size_t TSharedMemory::size() const
{
    return _size;
}
