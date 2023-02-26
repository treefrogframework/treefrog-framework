/* Copyright (c) 2022, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsharedmemorykvsdriver.h"
#include "tsharedmemoryallocator.h"
#include "tsystemglobal.h"


TSharedMemoryKvsDriver::TSharedMemoryKvsDriver()
{}


TSharedMemoryKvsDriver::~TSharedMemoryKvsDriver()
{
    delete _allocator;
}


static QString parseParameter(const QString &options, const QString &key)
{
    QString val;
    for (QString param : options.split(";", Tf::SkipEmptyParts)) {
        param = param.trimmed();
        if (param.startsWith(key + "=")) {
            val = param.mid(key.length() + 1);
            break;
        }
    }
    return val;
}


static size_t memorySize(const QString &options)
{
    const QRegularExpression re("^([\\d\\.]+)([GgMmKk]*)$");
    auto sizestr = parseParameter(options, "MEMORY_SIZE");

    auto match = re.match(sizestr);
    if (!match.hasMatch()) {
        return 0;
    }

    size_t size = 0;
    double d = match.captured(1).toDouble();
    QString postfix = match.captured(2).toUpper();

    if (postfix == "G") {
        size = d * 1024 * 1024 * 1024;
    } else if (postfix == "M") {
        size = d * 1024 * 1024;
    } else if (postfix == "K") {
        size = d * 1024;
    } else {
        size = d;
    }
    return size;
}


bool TSharedMemoryKvsDriver::open(const QString &db, const QString &, const QString &, const QString &, quint16, const QString &)
{
    _name = db;

    if (_name.isEmpty()) {
        return false;
    }

    if (_allocator) {
        return true;
    }

    _allocator = TSharedMemoryAllocator::attach(_name);
    if (_allocator) {
        tSystemDebug("SharedMemory attach.  name:%s  size:%zu", qUtf8Printable(_name), _allocator->mapSize());
        _size = _allocator->mapSize();
    } else {
       tSystemError("SharedMemory attach error.  name:%s", qUtf8Printable(_name));
    }
    return true;
}


void TSharedMemoryKvsDriver::close()
{
    // do nothing
}


void *TSharedMemoryKvsDriver::malloc(uint size)
{
    return _allocator ? _allocator->malloc(size) : nullptr;
}


void *TSharedMemoryKvsDriver::calloc(uint num, uint nsize)
{
    return _allocator ? _allocator->calloc(num, nsize) : nullptr;
}


void *TSharedMemoryKvsDriver::realloc(void *ptr, uint size)
{
    return _allocator ? _allocator->realloc(ptr, size) : nullptr;
}


void TSharedMemoryKvsDriver::free(void *ptr)
{
    if (_allocator) {
        _allocator->free(ptr);
    }
}


uint TSharedMemoryKvsDriver::allocSize(const void *ptr) const
{
    return _allocator ? _allocator->allocSize(ptr) : 0;
}


size_t TSharedMemoryKvsDriver::mapSize() const
{
    return _allocator ? _allocator->mapSize() : 0;
}


void *TSharedMemoryKvsDriver::origin() const
{
    return _allocator ? _allocator->origin() : nullptr;
}


void TSharedMemoryKvsDriver::initialize(const QString &db, const QString &options)
{
    auto size = memorySize(options);

    if (db.isEmpty()) {
        tSystemError("SharedMemory: Empty name  [%s:%d]", __FILE__, __LINE__);
        return;
    }

    if (size == 0) {
        tSystemDebug("options: %s", qUtf8Printable(options));
        tSystemWarn("SharedMemory: Invalid memory size. Changed to 256MB.");
        size = 256 * 1024 * 1024;
    }

    TSharedMemoryAllocator::initialize(db, size);
}


void TSharedMemoryKvsDriver::cleanup()
{
    if (!_name.isEmpty()) {
        TSharedMemoryAllocator::unlink(_name);
    }
}


bool TSharedMemoryKvsDriver::lockForRead()
{
    return _allocator->lockForRead();
}


bool TSharedMemoryKvsDriver::lockForWrite()
{
    return _allocator->lockForWrite();
}


bool TSharedMemoryKvsDriver::unlock()
{
    return _allocator->unlock();
}
