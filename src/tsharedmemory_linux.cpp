/* Copyright (c) 2022, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsharedmemory.h"
#include <TSystemGlobal>
#include <pthread.h>
#include <errno.h>


bool TSharedMemory::initRwlock(header_t *header) const
{
    int res = -1;
    pthread_rwlockattr_t attr;

    if (header) {
        res = pthread_rwlockattr_init(&attr);
        Q_ASSERT(!res);
        res = pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);  // Linux only
        Q_ASSERT(!res);
        if ((res = pthread_rwlock_init(&header->rwlock, &attr)) < 0) {
            tSystemError("pthread_rwlock_init failed: {}", (const char*)strerror(errno));
        }
    }
    return !res;
}


void TSharedMemory::releaseRwlock(header_t *header) const
{
    if (header) {
        pthread_rwlock_destroy(&header->rwlock);
    }
}


bool TSharedMemory::lockForRead()
{
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
                initRwlock(header);
            }
        }
    }
    header->lockcounter++;
    return true;
}


bool TSharedMemory::lockForWrite()
{
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
                releaseRwlock(header);
                initRwlock(header);
            }
        }
    }
    header->lockcounter++;
    return true;
}


bool TSharedMemory::unlock()
{
    header_t *header = (header_t *)_ptr;
    pthread_rwlock_unlock(&header->rwlock);
    return true;
}
