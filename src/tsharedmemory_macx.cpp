/* Copyright (c) 2025, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsharedmemory.h"
#include <semaphore.h>
#include <fcntl.h>      // O_CREAT, O_EXCL
#include <sys/stat.h>   // mode constants
#include <errno.h>


void TSharedMemory::initRwlock(header_t *) const
{
    const QByteArray SEM_NAME = _name + "_global_lock";

    sem_t* sem = sem_open(SEM_NAME.data(), O_CREAT | O_EXCL, 0644, 1);
    if (sem == SEM_FAILED) {
        if (errno == EEXIST) {
            return true;
        } else {
            tSystemError("sem_open (init) failed: {}", strerror(errno));
            return false;
        }
    }

    tSystemDebug("Semaphore initialized");
    sem_close(sem);
    return true;
}


bool TSharedMemory::lockForRead()
{
    const QByteArray SEM_NAME = _name + "_global_lock";

    sem_t* sem = sem_open(SEM_NAME.data(), 0);
    if (sem == SEM_FAILED) {
        tSystemError("sem_open (lock) failed: {}", strerror(errno));
        return false;
    }

    if (sem_wait(sem) < 0) {
        tSystemError("sem_wait failed: {}", strerror(errno));
        sem_close(sem);
        return false;
    }

    sem_close(sem);
    return true;
}


bool TSharedMemory::lockForWrite()
{
    // Same as lockForRead()
    return lockForRead();
}


bool TSharedMemory::unlock()
{
    const QByteArray SEM_NAME = _name + "_global_lock";

    sem_t* sem = sem_open(SEM_NAME.data(), 0);  // 既存を開く
    if (sem == SEM_FAILED) {
        tSystemError("sem_open (unlock) failed: {}", strerror(errno));
        return false;
    }

    if (sem_post(sem) < 0) {
        tSystemError("sem_post failed: {}", strerror(errno));
        sem_close(sem);
        return false;
    }

    sem_close(sem);
    return true;
}
