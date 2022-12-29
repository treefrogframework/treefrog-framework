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
#include <pthread.h>
#include <time.h>

struct header_t {
    pthread_rwlock_t rwlock;
    uint lockcounter {0};
};


static void rwlock_init(pthread_rwlock_t *rwlock)
{
    pthread_rwlockattr_t attr;

    int res = pthread_rwlockattr_init(&attr);
    Q_ASSERT(!res);
    res = pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    Q_ASSERT(!res);
    res = pthread_rwlock_init(rwlock, &attr);
    Q_ASSERT(!res);
}


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
    static const header_t INIT_HEADER = []() {
        header_t header;
        rwlock_init(&header.rwlock);
        return header;
    }();

    if (_ptr || size == 0 || _name.isEmpty()) {
        return false;
    }

    struct stat st;

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

    std::memcpy(_ptr, &INIT_HEADER, sizeof(INIT_HEADER));
    _size = size;
    tSystemDebug("SharedMemory created.  name:%s size:%zu", qUtf8Printable(_name), _size);
    return true;

error:
    tSystemError("SharedMemory create error.  name:%s size:%zu [%s:%d]", qUtf8Printable(_name), size, __FILE__, __LINE__);

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
    shm_unlink(qUtf8Printable(_name));
    tSystemDebug("SharedMemory unlinked.  name:%s", qUtf8Printable(_name));
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
    tSystemDebug("SharedMemory attached.  name:%s size:%zu", qUtf8Printable(_name), _size);
    return true;

error:
    tSystemError("SharedMemory attach error  [%s:%d]", __FILE__, __LINE__);

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


bool TSharedMemory::lockForRead()
{
#ifdef Q_OS_LINUX
    struct timespec timeout;
    header_t *header = (header_t *)_ptr;

    while (pthread_rwlock_tryrdlock(&header->rwlock) == EBUSY) {
        uint cnt = header->lockcounter;
        timespec_get(&timeout, TIME_UTC);
        timeout.tv_sec += 1;  // 1sec

        int res = pthread_rwlock_timedrdlock(&header->rwlock, &timeout);
        if (!res) {
            // success
            break;
        } else {
            if (res == ETIMEDOUT && header->lockcounter == cnt) {
                // resets rwlock object
                rwlock_init(&header->rwlock);
            }
        }
    }
    header->lockcounter++;
    return true;
#else
    header_t *header = (header_t *)_ptr;
    return pthread_rwlock_rdlock(&header->rwlock) == 0;
#endif
}


bool TSharedMemory::lockForWrite()
{
#ifdef Q_OS_LINUX
    struct timespec timeout;
    header_t *header = (header_t *)_ptr;

    while (pthread_rwlock_trywrlock(&header->rwlock) == EBUSY) {
        uint cnt = header->lockcounter;
        timespec_get(&timeout, TIME_UTC);
        timeout.tv_sec += 1;  // 1sec

        int res = pthread_rwlock_timedwrlock(&header->rwlock, &timeout);
        if (!res) {
            // success
            break;
        } else {
            if (res == ETIMEDOUT && header->lockcounter == cnt) {
                // resets rwlock object
                rwlock_init(&header->rwlock);
            }
        }
    }
    header->lockcounter++;
    return true;
#else
    header_t *header = (header_t *)_ptr;
    return pthread_rwlock_wrlock(&header->rwlock) == 0;
#endif
}


bool TSharedMemory::unlock()
{
    header_t *header = (header_t *)_ptr;
    pthread_rwlock_unlock(&header->rwlock);
    return true;
}
