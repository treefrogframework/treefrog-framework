#include "tsharedmemoryhash.h"
#include "tshm.h"
#include "tsharedmemoryallocator.h"
#include <QDataStream>
#include <ctime>
#include <pthread.h>

#define FREE ((void *)-1)

struct hash_header_t {
    uint64_t hashtg {0};
    pthread_rwlock_t rwlock;
    uint lockcounter {0};
    uint tableSize {1024};
    uint count {0};
    uint freeCount {0};

    uint64_t *hashg() { return hashtg ? (uint64_t *)((caddr_t)this + hashtg) : nullptr; }

    void setHashg(void *p)
    {
        hashtg = p ? (uint64_t)p - (uint64_t)this : 0;
    }

    void *bucketPtr(uint index)
    {
        if (index < tableSize) {
            uint64_t g = *(hashg() + index);
            return (g && g != -1UL) ? (caddr_t)this + g : (void *)g;
        } else {
            Q_ASSERT(0);
        }
        return nullptr;
    }

    void setBucketPtr(uint index, void *ptr)
    {
        if (index < tableSize) {
            *(hashg() + index) = (ptr && ptr != FREE) ? (uint64_t)ptr - (uint64_t)this : (uint64_t)ptr;
        } else {
            Q_ASSERT(0);
        }
    }
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


TSharedMemoryHash::TSharedMemoryHash(const QString &name, size_t size) :
    _allocator(new TSharedMemoryAllocator(name, size))
{
    static const hash_header_t INIT_HEADER = []() {
        hash_header_t header;
        rwlock_init(&header.rwlock);
        return header;
    }();

    _h = (hash_header_t *)_allocator->origin();

    if (_allocator->isNew()) {
        void *ptr = _allocator->malloc(sizeof(INIT_HEADER));
        Q_ASSERT(ptr == _h);
        std::memcpy(_h, &INIT_HEADER, sizeof(INIT_HEADER));
        ptr = _allocator->calloc(_h->tableSize, sizeof(uint64_t));
        _h->setHashg(ptr);
        Q_ASSERT(ptr);
    }
}


TSharedMemoryHash::~TSharedMemoryHash()
{
    delete _allocator;
}


bool TSharedMemoryHash::insert(const QByteArray &key, const QByteArray &value)
{
    if (key.isEmpty()) {
        return false;
    }

    QByteArray data;
    QDataStream ds(&data, QIODeviceBase::WriteOnly);
    ds << Bucket{key, value};

    lockForWrite();  // lock

    Bucket bucket;
    QByteArray buf;
    uint idx = index(key);
    bool ret = false;

    for (uint i = 0; i < _h->tableSize; i++) {
        void *pbucket = _h->bucketPtr(idx);
        if (pbucket && pbucket != FREE) {
            // checks key
            int alcsize = _allocator->allocSize(pbucket);
            if (alcsize <= 0) {
                // error
                Q_ASSERT(0);
                break;
            }

            buf.setRawData((char *)pbucket, alcsize);
            QDataStream dsb(buf);
            dsb >> bucket;

            if (bucket.key != key) {
                idx = next(idx);
                continue;
            }

            _allocator->free(pbucket);
            (_h->count)--;
        }

        // Inserts data
        void *newbucket = _allocator->malloc(data.size());
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


uint TSharedMemoryHash::find(const QByteArray &key, Bucket &bucket) const
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
            int alcsize = _allocator->allocSize(pbucket);
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


bool TSharedMemoryHash::find(uint index, Bucket &bucket) const
{
    if (index >= tableSize()) {
        return false;
    }

    void *pbucket = _h->bucketPtr(index);
    if (!pbucket || pbucket == FREE) {
        return false;
    }

    int alcsize = _allocator->allocSize(pbucket);
    if (alcsize <= 0) {
        Q_ASSERT(0);
        return false;
    }

    QByteArray buf((char *)pbucket, alcsize);
    QDataStream ds(buf);
    ds >> bucket;
    return true;
}


QByteArray TSharedMemoryHash::value(const QByteArray &key, const QByteArray &defaultValue) const
{
    Bucket bucket;
    lockForRead();  // lock
    uint idx = find(key, bucket);
    unlock();  // unlock
    return (idx < tableSize()) ? bucket.value : defaultValue;
}


QByteArray TSharedMemoryHash::take(const QByteArray &key, const QByteArray &defaultValue)
{
    Bucket bucket;
    lockForWrite();  // lock
    uint idx = find(key, bucket);
    if (idx < tableSize()) {
        _allocator->free(_h->bucketPtr(idx));
        _h->setBucketPtr(idx, FREE);
        (_h->count)--;
        (_h->freeCount)++;
    }
    unlock();  // unlock
    return (idx < tableSize()) ? bucket.value : defaultValue;
}


bool TSharedMemoryHash::remove(const QByteArray &key)
{
    Bucket bucket;
    lockForWrite();  // lock
    uint idx = find(key, bucket);
    remove(idx);
    unlock();  // unlock
    return (idx < tableSize());
}


void TSharedMemoryHash::remove(uint index)
{
    if (index < tableSize()) {
        void *ptr = _h->bucketPtr(index);
        if (ptr && ptr != FREE) {
            _allocator->free(ptr);
            _h->setBucketPtr(index, FREE);
            (_h->count)--;
            (_h->freeCount)++;
        }
    }
}


uint TSharedMemoryHash::count() const
{
    return _h->count;
}


uint TSharedMemoryHash::tableSize() const
{
    return _h->tableSize;
}


float TSharedMemoryHash::loadFactor() const
{
    return (count() + _h->freeCount) / (float)_h->tableSize;
}


void TSharedMemoryHash::clear()
{
    lockForWrite();  // lock
    for (uint i = 0; i < _h->tableSize; i++) {
        void *pbucket = _h->bucketPtr(i);
        if (pbucket == FREE) {
            (_h->freeCount)--;
        } else if (pbucket) {
            _allocator->free(pbucket);
            (_h->count)--;
        }
        _h->setBucketPtr(i, nullptr);
    }
    Q_ASSERT(_h->count == 0);
    Q_ASSERT(_h->freeCount == 0);
    unlock();  // unlock
}


void TSharedMemoryHash::rehash()
{
    Bucket bucket;
    QByteArray buf;

    if (loadFactor() < 0.2) {
        // do nothing
        return;
    }
    uint64_t *const oldt = _h->hashg();
    int oldsize = _h->tableSize;

    // Creates new table
    if (count() / (float)_h->tableSize > 0.5) {
        _h->tableSize = oldsize * 2;  // new table size
    }

    auto *ptr = _allocator->calloc(_h->tableSize, sizeof(uint64_t));
    _h->setHashg(ptr);
    Q_ASSERT(ptr);

    for (int i = 0; i < oldsize; i++) {
        uint64_t g = *(oldt + i);
        void *pbucket = (g && g != -1UL) ? (caddr_t)_h + g : nullptr;

        if (!pbucket) {
            continue;
        }

        int alcsize = _allocator->allocSize(pbucket);
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
    //_allocator->dump();
    _allocator->free(oldt);
}


uint TSharedMemoryHash::index(const QByteArray &key) const
{
    return qHash(key) % _h->tableSize;
}


uint TSharedMemoryHash::next(uint index) const
{
    return (index + 1) % _h->tableSize;
}


void TSharedMemoryHash::lockForRead() const
{
    std::timespec timeout;

    while (pthread_rwlock_tryrdlock(&_h->rwlock) == EBUSY) {
        uint cnt = _h->lockcounter;
        std::timespec_get(&timeout, TIME_UTC);
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
}


void TSharedMemoryHash::lockForWrite()
{
    std::timespec timeout;

    while (pthread_rwlock_trywrlock(&_h->rwlock) == EBUSY) {
        uint cnt = _h->lockcounter;
        std::timespec_get(&timeout, TIME_UTC);
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
}


void TSharedMemoryHash::unlock() const
{
    pthread_rwlock_unlock(&_h->rwlock);
}


TSharedMemoryHash::WriteLockingIterator TSharedMemoryHash::begin()
{
    WriteLockingIterator it(this, (uint)-1);
    it.search();
    return it;
}


TSharedMemoryHash::WriteLockingIterator TSharedMemoryHash::end()
{
    return WriteLockingIterator(this, tableSize());
}


TSharedMemoryHash::WriteLockingIterator::WriteLockingIterator(TSharedMemoryHash *hash, uint it) :
    _hash(hash), _it(it)
{
    if (_it < _hash->tableSize() || _it == (uint)-1) {
        _hash->lockForWrite();
        _locked = true;
    }
}


TSharedMemoryHash::WriteLockingIterator::~WriteLockingIterator()
{
    if (_locked) {
        _hash->unlock();
    }
}


const QByteArray &TSharedMemoryHash::WriteLockingIterator::key() const
{
    return _tmpbk.key;
}


const QByteArray &TSharedMemoryHash::WriteLockingIterator::value() const
{
    return _tmpbk.value;
}


const QByteArray &TSharedMemoryHash::WriteLockingIterator::operator*() const
{
    return value();
}


void TSharedMemoryHash::WriteLockingIterator::search()
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


TSharedMemoryHash::WriteLockingIterator &TSharedMemoryHash::WriteLockingIterator::operator++()
{
    search();
    return *this;
}


void TSharedMemoryHash::WriteLockingIterator::remove()
{
    if (_it < _hash->tableSize()) {
        _hash->remove(_it);
    }
    _tmpbk.clear();
}
