/* Copyright (c) 2022, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsharedmemorykvs.h"
#include "tsharedmemorykvsdriver.h"
#include <TActionContext>
#include <TSystemGlobal>
#include <QDataStream>
#include <cstring>


#define FREE ((void *)-1)

struct hash_header_t {
    uintptr_t hashtg {0};
    uint tableSize {1024};
    uint count {0};
    uint freeCount {0};

    uintptr_t *hashg() { return hashtg ? (uintptr_t *)((uintptr_t)this + hashtg) : nullptr; }

    void setHashg(void *p)
    {
        hashtg = p ? (uintptr_t)p - (uintptr_t)this : 0;
    }

    void *bucketPtr(uint index)
    {
        if (index < tableSize) {
            uintptr_t g = *(hashg() + index);
            return (g && g != -1UL) ? (char *)this + g : (void *)g;
        } else {
            Q_ASSERT(0);
        }
        return nullptr;
    }

    void setBucketPtr(uint index, void *ptr)
    {
        if (index < tableSize) {
            *(hashg() + index) = (ptr && ptr != FREE) ? (uintptr_t)ptr - (uintptr_t)this : (uintptr_t)ptr;
        } else {
            Q_ASSERT(0);
        }
    }
};


// static void rwlock_init(pthread_rwlock_t *rwlock)
// {
//     pthread_rwlockattr_t attr;

//     int res = pthread_rwlockattr_init(&attr);
//     Q_ASSERT(!res);
//     res = pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
//     Q_ASSERT(!res);
//     res = pthread_rwlock_init(rwlock, &attr);
//     Q_ASSERT(!res);
// }

/*!
  Constructs a TSharedMemoryKvs object.
*/
TSharedMemoryKvs::TSharedMemoryKvs() :
    _database(Tf::currentDatabaseContext()->getKvsDatabase(Tf::KvsEngine::SharedMemory))
{
    _h = (hash_header_t *)driver()->origin();
}


TSharedMemoryKvs::TSharedMemoryKvs(Tf::KvsEngine engine) :
    _database(Tf::currentDatabaseContext()->getKvsDatabase(engine))
{
    _h = (hash_header_t *)driver()->origin();
}


TSharedMemoryKvs::~TSharedMemoryKvs()
{ }


bool TSharedMemoryKvs::initialize(const QString &name, const QString &options)
{
    // static const hash_header_t INIT_HEADER = []() {
    //     hash_header_t header;
    //     rwlock_init(&header.rwlock);
    //     return header;
    // }();
    hash_header_t hashheader;

    TSharedMemoryKvsDriver::initialize(name, options);
    TSharedMemoryKvsDriver driver;
    driver.open(name, QString(), QString(), QString(), 0, options);
    hash_header_t *header = (hash_header_t *)driver.origin();

    void *ptr = driver.malloc(sizeof(hashheader));
    Q_ASSERT(ptr == header);
    std::memcpy(header, &hashheader, sizeof(hashheader));
    ptr = driver.calloc(header->tableSize, sizeof(uintptr_t));
    header->setHashg(ptr);
    Q_ASSERT(ptr);
    return true;
}


void TSharedMemoryKvs::cleanup()
{
    driver()->cleanup();
}


bool TSharedMemoryKvs::set(const QByteArray &key, const QByteArray &value, int seconds)
{
    if (key.isEmpty() || seconds <= 0) {
        return false;
    }

    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    qint64 expires = Tf::getMSecsSinceEpoch() + seconds * 1000;
    ds << Bucket{key, value, expires};

    lockForWrite();  // lock

    Bucket bucket;
    QByteArray buf;
    uint idx = index(key);
    bool ret = false;

    for (uint i = 0; i < _h->tableSize; i++) {
        void *pbucket = _h->bucketPtr(idx);
        if (pbucket && pbucket != FREE) {
            // checks key
            int alcsize = driver()->allocSize(pbucket);
            if (alcsize <= 0) {
                // error
                Q_ASSERT(0);
                break;
            }

            buf.setRawData((char *)pbucket, alcsize);
            QDataStream dsb(buf);
            dsb >> bucket;

            if (bucket.expires > Tf::getMSecsSinceEpoch() && bucket.key != key) {
                idx = next(idx);
                continue;
            }

            driver()->free(pbucket);
            (_h->count)--;
        }

        // Inserts data
        void *newbucket = driver()->malloc(data.size());
        if (!newbucket) {
            tError("Not enough space/cannot allocate memory.  errno:%d", errno);
            break;
        }

        std::memcpy(newbucket, data.data(), data.size());
        _h->setBucketPtr(idx, newbucket);
        (_h->count)++;

        if (pbucket == FREE) {
            (_h->freeCount)--;
        }

        // Rehash
        if (loadFactor() > 0.8) {
            rehash();
        }

        ret = true;
        break;
    }

    unlock();  // unlock
    return ret;
}


uint TSharedMemoryKvs::find(const QByteArray &key, Bucket &bucket) const
{
    if (key.isEmpty()) {
        return -1;
    }

    uint idx = index(key);
    QByteArray buf;

    for (uint i = 0; i < _h->tableSize; i++) {
        void *pbucket = _h->bucketPtr(idx);
        if (!pbucket) {
            break;
        }

        if (pbucket != FREE) {
            int alcsize = driver()->allocSize(pbucket);
            if (alcsize <= 0) {
                Q_ASSERT(0);
                break;
            }

            buf.setRawData((char *)pbucket, alcsize);
            QDataStream ds(buf);
            ds >> bucket;

            if (bucket.key == key) {
                // Found
                return idx;
            }
        }
        idx = next(idx);
    }

    return -1;
}


bool TSharedMemoryKvs::find(uint index, Bucket &bucket) const
{
    if (index >= tableSize()) {
        return false;
    }

    void *pbucket = _h->bucketPtr(index);
    if (!pbucket || pbucket == FREE) {
        return false;
    }

    int alcsize = driver()->allocSize(pbucket);
    if (alcsize <= 0) {
        Q_ASSERT(0);
        return false;
    }

    QByteArray buf((char *)pbucket, alcsize);
    QDataStream ds(buf);
    ds >> bucket;
    return true;
}


QByteArray TSharedMemoryKvs::get(const QByteArray &key)
{
    Bucket bucket;
    lockForRead();  // lock
    uint idx = find(key, bucket);
    unlock();  // unlock
    return (idx < tableSize() && bucket.expires > Tf::getMSecsSinceEpoch()) ? bucket.value : QByteArray();
}


bool TSharedMemoryKvs::remove(const QByteArray &key)
{
    Bucket bucket;
    lockForWrite();  // lock
    uint idx = find(key, bucket);
    remove(idx);
    unlock();  // unlock
    return (idx < tableSize());
}


void TSharedMemoryKvs::remove(uint index)
{
    if (index < tableSize()) {
        void *ptr = _h->bucketPtr(index);
        if (ptr && ptr != FREE) {
            driver()->free(ptr);
            _h->setBucketPtr(index, FREE);
            (_h->count)--;
            (_h->freeCount)++;
        }
    }
}


uint TSharedMemoryKvs::count() const
{
    return _h->count;
}


uint TSharedMemoryKvs::tableSize() const
{
    return _h->tableSize;
}


float TSharedMemoryKvs::loadFactor() const
{
    return (count() + _h->freeCount) / (float)_h->tableSize;
}


void TSharedMemoryKvs::clear()
{
    lockForWrite();  // lock
    for (uint i = 0; i < _h->tableSize; i++) {
        void *pbucket = _h->bucketPtr(i);
        if (pbucket == FREE) {
            (_h->freeCount)--;
        } else if (pbucket) {
            driver()->free(pbucket);
            (_h->count)--;
        }
        _h->setBucketPtr(i, nullptr);
    }
    Q_ASSERT(_h->count == 0);
    Q_ASSERT(_h->freeCount == 0);
    unlock();  // unlock
}


void TSharedMemoryKvs::gc()
{
    for (auto it = begin(); it != end(); ++it) {
        if (it.isExpired()) {
            it.remove();
        }
    }

    lockForWrite();  // lock
    rehash();
    unlock();  // unlock
}


void TSharedMemoryKvs::rehash()
{
    if (loadFactor() < 0.2) {
        // do nothing
        return;
    }

    Bucket bucket;
    QByteArray buf;
    uintptr_t *const oldt = _h->hashg();
    int oldsize = _h->tableSize;

    // Creates new table
    if (count() / (float)_h->tableSize > 0.5) {
        _h->tableSize = oldsize * 2;  // new table size
    }

    auto *ptr = driver()->calloc(_h->tableSize, sizeof(uintptr_t));
    _h->setHashg(ptr);
    Q_ASSERT(ptr);

    for (int i = 0; i < oldsize; i++) {
        uintptr_t g = *(oldt + i);
        void *pbucket = (g && g != -1UL) ? (char *)_h + g : nullptr;

        if (!pbucket) {
            continue;
        }

        int alcsize = driver()->allocSize(pbucket);
        if (alcsize <= 0) {
            Q_ASSERT(0);
            continue;
        }

        buf.setRawData((char *)pbucket, alcsize);
        QDataStream ds(buf);
        ds >> bucket;

        int newidx = index(bucket.key);
        while (_h->bucketPtr(newidx)) {
            newidx = next(newidx);
        }
        _h->setBucketPtr(newidx, pbucket);
    }

    _h->freeCount = 0;
    //driver()->dump();
    driver()->free(oldt);
}


uint TSharedMemoryKvs::index(const QByteArray &key) const
{
    return qHash(key) % _h->tableSize;
}


uint TSharedMemoryKvs::next(uint index) const
{
    return (index + 1) % _h->tableSize;
}


bool TSharedMemoryKvs::lockForRead()
{
#if 1
    return driver()->lockForRead();
#else
    struct timespec timeout;

    while (pthread_rwlock_tryrdlock(&_h->rwlock) == EBUSY) {
        uint cnt = _h->lockcounter;
        timespec_get(&timeout, TIME_UTC);
        timeout.tv_sec += 1;  // 1sec

        int res = pthread_rwlock_timedrdlock(&_h->rwlock, &timeout);
        if (!res) {
            break;
        } else {
            if (res == ETIMEDOUT && _h->lockcounter == cnt) {
                // resets rwlock object
                rwlock_init(&_h->rwlock);
            }
        }
    }
    _h->lockcounter++;
#endif
}


bool TSharedMemoryKvs::lockForWrite()
{
#if 1
    return driver()->lockForWrite();
#else
    struct timespec timeout;

    while (pthread_rwlock_trywrlock(&_h->rwlock) == EBUSY) {
        uint cnt = _h->lockcounter;
        timespec_get(&timeout, TIME_UTC);
        timeout.tv_sec += 1;  // 1sec

        int res = pthread_rwlock_timedwrlock(&_h->rwlock, &timeout);
        if (!res) {
            break;
        } else {
            if (res == ETIMEDOUT && _h->lockcounter == cnt) {
                // resets rwlock object
                rwlock_init(&_h->rwlock);
            }
        }
    }
    _h->lockcounter++;
#endif
}


bool TSharedMemoryKvs::unlock()
{
#if 1
    return driver()->unlock();
#else
    pthread_rwlock_unlock(&_h->rwlock);
#endif
}


/*!
  Returns the Memcached driver associated with the TSharedMemoryKvs object.
*/
TSharedMemoryKvsDriver *TSharedMemoryKvs::driver()
{
#ifdef TF_NO_DEBUG
    return (TSharedMemoryKvsDriver *)_database.driver();
#else
    if (!_database.driver()) {
        return nullptr;
    }

    TSharedMemoryKvsDriver *driver = dynamic_cast<TSharedMemoryKvsDriver *>(_database.driver());
    if (!driver) {
        throw RuntimeException("cast error", __FILE__, __LINE__);
    }
    return driver;
#endif
}

/*!
  Returns the Memcached driver associated with the TSharedMemoryKvs object.
*/
const TSharedMemoryKvsDriver *TSharedMemoryKvs::driver() const
{
#ifdef TF_NO_DEBUG
    return (const TSharedMemoryKvsDriver *)_database.driver();
#else
    if (!_database.driver()) {
        return nullptr;
    }

    const TSharedMemoryKvsDriver *driver = dynamic_cast<const TSharedMemoryKvsDriver *>(_database.driver());
    if (!driver) {
        throw RuntimeException("cast error", __FILE__, __LINE__);
    }
    return driver;
#endif
}


TSharedMemoryKvs::WriteLockingIterator TSharedMemoryKvs::begin()
{
    WriteLockingIterator it(this, (uint)-1);
    it.search();
    return it;
}


TSharedMemoryKvs::WriteLockingIterator TSharedMemoryKvs::end()
{
    return WriteLockingIterator(this, tableSize());
}


TSharedMemoryKvs::WriteLockingIterator::WriteLockingIterator(TSharedMemoryKvs *hash, uint it) :
    _hash(hash), _it(it)
{
    if (_it < _hash->tableSize() || _it == (uint)-1) {
        _hash->lockForWrite();
        _locked = true;
    }
}


TSharedMemoryKvs::WriteLockingIterator::~WriteLockingIterator()
{
    if (_locked) {
        _hash->unlock();
    }
}


const QByteArray &TSharedMemoryKvs::WriteLockingIterator::key() const
{
    return _tmpbk.key;
}


const QByteArray &TSharedMemoryKvs::WriteLockingIterator::value() const
{
    return _tmpbk.value;
}


bool TSharedMemoryKvs::WriteLockingIterator::isExpired() const
{
    return _tmpbk.isExpired();
}


const QByteArray &TSharedMemoryKvs::WriteLockingIterator::operator*() const
{
    return value();
}


void TSharedMemoryKvs::WriteLockingIterator::search()
{
    _tmpbk.clear();

    if (_hash->count() == 0) {
        _it = _hash->tableSize();
        return;
    }

    if (_it == _hash->tableSize()) {
        return;
    }

    while (++_it < _hash->tableSize()) {
        if (_hash->find(_it, _tmpbk)) {
            break;
        }
    }
}


TSharedMemoryKvs::WriteLockingIterator &TSharedMemoryKvs::WriteLockingIterator::operator++()
{
    search();
    return *this;
}


void TSharedMemoryKvs::WriteLockingIterator::remove()
{
    if (_it < _hash->tableSize()) {
        _hash->remove(_it);
    }
    _tmpbk.clear();
}
