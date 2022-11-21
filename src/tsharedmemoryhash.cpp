#include "tsharedmemoryhash.h"
#include "tshm.h"
#include "tsharedmemoryallocator.h"
#include <QDataStream>

#define FREE ((void *)-1)


class Bucket {
public:
    QByteArray key;
    QByteArray value;

    friend QDataStream &operator<<(QDataStream &ds, const Bucket &bucket);
    friend QDataStream &operator>>(QDataStream &ds, Bucket &bucket);
};


class Locker {
public:
    Locker(TSharedMemoryAllocator *allocator) : _allocator(allocator){ _allocator->lock(); }
    ~Locker() { _allocator->unlock(); }
private:
    TSharedMemoryAllocator *_allocator {nullptr};
};


inline QDataStream &operator<<(QDataStream &ds, const Bucket &bucket)
{
    ds << bucket.key << bucket.value;
    return ds;
}


inline QDataStream &operator>>(QDataStream &ds, Bucket &bucket)
{
    ds >> bucket.key >> bucket.value;
    return ds;
}


TSharedMemoryHash::TSharedMemoryHash(const QString &name, size_t size) :
    _allocator(new TSharedMemoryAllocator(name, size))
{
    static const hash_header_t INIT_HEADER;

    Locker locker(_allocator);
    _h = (hash_header_t *)_allocator->origin();

    if (_allocator->isNew()) {
        void *ptr = _allocator->malloc(sizeof(INIT_HEADER));
        Q_ASSERT(ptr == _h);
        memcpy(_h, &INIT_HEADER, sizeof(INIT_HEADER));
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

    Locker locker(_allocator);
    QByteArray data;
    QDataStream ds(&data, QIODeviceBase::WriteOnly);
    ds << Bucket{key, value};

    Bucket bucket;
    QByteArray buf;
    uint idx = index(key);

    for (int i = 0; i < _h->tableSize; i++) {
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

        memcpy(newbucket, data.data(), data.size());
        _h->setBucketPtr(idx, newbucket);
        (_h->count)++;

        if (pbucket == FREE) {
            (_h->freeCount)--;
        }

        // Rehash
        if (loadFactor() > 0.8) {
            rehash();
        }
        return true;
    }

    return false;
}


int TSharedMemoryHash::find(const QByteArray &key, Bucket &bucket) const
{
    if (key.isEmpty()) {
        return -1;
    }

    uint idx = index(key);
    QByteArray buf;

    for (int i = 0; i < _h->tableSize; i++) {
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


QByteArray TSharedMemoryHash::value(const QByteArray &key, const QByteArray &defaultValue) const
{
    Locker locker(_allocator);

    Bucket bucket;
    int idx = find(key, bucket);
    return (idx >= 0) ? bucket.value : defaultValue;
}


QByteArray TSharedMemoryHash::take(const QByteArray &key, const QByteArray &defaultValue)
{
    Locker locker(_allocator);

    Bucket bucket;
    int idx = find(key, bucket);
    if (idx >= 0) {
        _allocator->free(_h->bucketPtr(idx));
        _h->setBucketPtr(idx, FREE);
        (_h->count)--;
        (_h->freeCount)++;
    }
    return (idx >= 0) ? bucket.value : defaultValue;
}


bool TSharedMemoryHash::remove(const QByteArray &key)
{
    Locker locker(_allocator);

    Bucket bucket;
    int idx = find(key, bucket);
    if (idx >= 0) {
        _allocator->free(_h->bucketPtr(idx));
        _h->setBucketPtr(idx, FREE);
        (_h->count)--;
        (_h->freeCount)++;
    }
    return (idx >= 0);
}


int	TSharedMemoryHash::count() const
{
    return _h->count;
}


float TSharedMemoryHash::loadFactor() const
{
    return (count() + _h->freeCount) / (float)_h->tableSize;
}


void TSharedMemoryHash::clear()
{
    Locker locker(_allocator);

    for (int i = 0; i < _h->tableSize; i++) {
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
