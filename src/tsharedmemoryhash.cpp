#include "tsharedmemoryhash.h"
#include "tshm.h"
#include "tfmalloc.h"
#include <QDataStream>

#define FREE ((void *)-1)


class Bucket {
public:
    QByteArray key;
    QByteArray value;

    friend QDataStream &operator<<(QDataStream &ds, const Bucket &bucket);
    friend QDataStream &operator>>(QDataStream &ds, Bucket &bucket);
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


bool TSharedMemoryHash::insert(const QByteArray &key, const QByteArray &value)
{
    if (key.isEmpty()) {
        return false;
    }

    uint idx = index(key);

    QByteArray data;
    QDataStream ds(&data, QIODeviceBase::WriteOnly);
    ds << Bucket{key, value};

    Bucket bucket;
    QByteArray buf;

    for (;;) {
        void *pbucket = _h->bucketPtr(idx);
        if (pbucket && pbucket != FREE) {
            // checks key
            int alcsize = _allocator->allocSize(pbucket);
            if (alcsize <= 0) {
                // error
                Q_ASSERT(0);
                return false;
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
        pbucket = _allocator->malloc(data.size());
        if (!pbucket) {
            tError("Not enough space/cannot allocate memory.  errno:%d", errno);
            return false;
        }

        memcpy(pbucket, data.data(), data.size());
        _h->setBucketPtr(idx, pbucket);
        (_h->count)++;
        break;
    }

    // Rehash
    if (loadFactor() > 0.8) {
        rehash();
    }
    return true;
}


int TSharedMemoryHash::find(const QByteArray &key, Bucket &bucket) const
{
    if (key.isEmpty()) {
        return -1;
    }

    uint idx = index(key);
    QByteArray buf;

    for (;;) {
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
    Bucket bucket;
    int idx = find(key, bucket);
    return (idx >= 0) ? bucket.value : defaultValue;
}


QByteArray TSharedMemoryHash::take(const QByteArray &key, const QByteArray &defaultValue)
{
    Bucket bucket;
    int idx = find(key, bucket);
    if (idx >= 0) {
        _allocator->free(_h->bucketPtr(idx));
        _h->setBucketPtr(idx, FREE);
        (_h->count)--;
    }
    return (idx >= 0) ? bucket.value : defaultValue;
}


bool TSharedMemoryHash::remove(const QByteArray &key)
{
    Bucket bucket;
    int idx = find(key, bucket);
    if (idx >= 0) {
        _allocator->free(_h->bucketPtr(idx));
        _h->setBucketPtr(idx, FREE);
        (_h->count)--;
    }
    return (idx >= 0);
}


int	TSharedMemoryHash::count() const
{
    return _h->count;
}


float TSharedMemoryHash::loadFactor() const
{
    return count() / (float)_h->tableSize;
}


void TSharedMemoryHash::clear()
{
    for (int i = 0; i < _h->tableSize; i++) {
        void *pbucket = _h->bucketPtr(i);
        if (pbucket && pbucket != FREE) {
            _allocator->free(pbucket);
            (_h->count)--;
        }
        _h->setBucketPtr(i, nullptr);
    }
    Q_ASSERT(_h->count == 0);
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
    _h->tableSize = oldsize * 4;  // new table size
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

    //_allocator->dump();
    _allocator->free(oldt);
}
