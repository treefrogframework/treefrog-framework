/* Copyright (c) 2025, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsharedmemory.h"
#include <TSystemGlobal>
#include <semaphore.h>
#include <fcntl.h>      // O_CREAT, O_EXCL
#include <sys/stat.h>
#include <time.h>
#include <errno.h>


static inline QByteArray semaphoreName(const QString &name)
{
    return (name.startsWith("/") ? "" : "/") + name.toLatin1() + "_global_lock";
}


bool TSharedMemory::initRwlock(header_t *) const
{
    sem_t *sem = sem_open(semaphoreName(_name).data(), O_CREAT | O_EXCL, 0644, 1);
    if (sem == SEM_FAILED) {
        if (errno == EEXIST) {
            tSystemError("sem_open semaphore already exists: {}", semaphoreName(_name));
            return true;
        } else {
            tSystemError("sem_open (init) failed: {}", (const char*)strerror(errno));
            return false;
        }
    }

    tSystemDebug("Semaphore initialized");
    sem_close(sem);
    return true;
}


void TSharedMemory::releaseRwlock(header_t *) const
{
    sem_unlink(semaphoreName(_name).data());
}


bool TSharedMemory::lockForRead()
{
    // Use semaphore as a lock mechanism between processes.
    // PTHREAD_PROCESS_SHARED attribute is not supported on macos.

    sem_t *sem = SEM_FAILED;
    header_t *header = (header_t *)_ptr;
    uint cnt = header->lockcounter;

    auto sem_timedwait = [&](int msecs) {
        sem = sem_open(semaphoreName(_name).data(), 0);
        if (sem == SEM_FAILED) {
            tSystemError("sem_open (lock) failed: {}", (const char*)strerror(errno));
            return -1;
        }

        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(msecs);
        while (std::chrono::steady_clock::now() < deadline) {
            if (sem_trywait(sem) < 0) {
                if (errno == EAGAIN) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    continue;
                } else {
                    return -1;  // error
                }
            } else {
                header->lockcounter++;
                return 0;  // lock success
            }
        }
        tSystemError("sem_wait (lock) timed out: {}", semaphoreName(_name));
        return 1;  // timeout
    };

    int res;
    while ((res = sem_timedwait(1000)) == 1) {
        if (header->lockcounter == cnt) {  // timeout and same counter
            releaseRwlock(header);
            initRwlock(header);
        }
    }

    if (sem != SEM_FAILED) {
        sem_close(sem);
    }
    return !res;
}


bool TSharedMemory::lockForWrite()
{
    // Same as lockForRead()
    return lockForRead();
}


bool TSharedMemory::unlock()
{
    sem_t *sem = sem_open(semaphoreName(_name).data(), 0);
    if (sem == SEM_FAILED) {
        tSystemError("sem_open (unlock) failed: {}", (const char*)strerror(errno));
        return false;
    }

    if (sem_post(sem) < 0) {
        tSystemError("sem_post failed: {}", (const char*)strerror(errno));
        sem_close(sem);
        return false;
    }

    sem_close(sem);
    return true;
}
