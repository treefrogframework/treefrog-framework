/* Copyright (c) 2022, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsharedmemory.h"
#include "tsystemglobal.h"


TSharedMemory::TSharedMemory(const QString &name) :
    QSharedMemory(name)
{ }


TSharedMemory::~TSharedMemory()
{ }


bool TSharedMemory::create(size_t sz)
{
    if (QSharedMemory::create(sz)) {
        tSystemDebug("SharedMemory created.  name:%s size:%zu", qUtf8Printable(key()), sz);
        return true;
    }

    auto err = error();
    if (err == QSharedMemory::AlreadyExists && attach()) {
        if (size() >= sz) {
            tSystemWarn("SharedMemory already exists, attached.  name:%s size:%zu", qUtf8Printable(key()), size());
            return true;
        }
        detach();
    }

    tSystemError("SharedMemory create error [%d].  name:%s size:%zu [%s:%d]", (int)err, qUtf8Printable(key()), sz, __FILE__, __LINE__);
    return false;
}


void TSharedMemory::unlink()
{
    // do nothing
}


bool TSharedMemory::attach()
{
    if (!QSharedMemory::attach()) {
        tSystemError("SharedMemory attach error  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    tSystemDebug("SharedMemory attached.  name:%s size:%zu", qUtf8Printable(key()), size());
    return true;
}


bool TSharedMemory::detach()
{
    return QSharedMemory::detach();
}


void *TSharedMemory::data()
{
    return QSharedMemory::data();
}


const void *TSharedMemory::data() const
{
    return QSharedMemory::data();
}


QString TSharedMemory::name() const
{
    return QSharedMemory::key();
}


size_t TSharedMemory::size() const
{
    return QSharedMemory::size();
}


bool TSharedMemory::lockForRead()
{
    return QSharedMemory::lock();
}


bool TSharedMemory::lockForWrite()
{
    return QSharedMemory::lock();
}


bool TSharedMemory::unlock()
{
    return QSharedMemory::unlock();
}
