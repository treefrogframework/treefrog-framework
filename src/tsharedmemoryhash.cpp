#include "tsharedmemoryhash.h"
#include "tshm.h"
#include "tfmalloc.h"
#include <QDataStream>

#define FREE ((void *)-1)


struct Bucket {
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


TSharedMemoryHash::TSharedMemoryHash(const QString &name, size_t size)
{
    static const hash_header_t INIT_HEADER;

    bool newmap;
    void *ptr = Tf::shmcreate(qUtf8Printable(name), size, &newmap);
    ptr = Tf::setbrk(ptr, size, newmap);
    _h = (hash_header_t *)((caddr_t)ptr + sizeof(Tf::alloc_header_t));
    qDebug() << "sizeof(Tf::alloc_header_t):" << sizeof(Tf::alloc_header_t);

    if (newmap) {
        ptr = Tf::smalloc(sizeof(INIT_HEADER));
        Q_ASSERT(ptr == _h);
        memcpy(_h, &INIT_HEADER, sizeof(INIT_HEADER));
        ptr = Tf::scalloc(_h->tableSize, sizeof(int64_t));
        _h->setHashg(ptr);
    }
}


void TSharedMemoryHash::insert(const QByteArray &key, const QByteArray &value)
{
    if (key.isEmpty()) {
        return;
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
            int alcsize = Tf::allocsize(pbucket);
            if (alcsize <= 0) {
                // error
                Q_ASSERT(0);
                return;
            }

            buf.setRawData((char *)pbucket, alcsize);
            QDataStream dsb(buf);
            dsb >> bucket;

            if (bucket.key != key) {
                idx = next(idx);
                continue;
            }

            Tf::sfree(pbucket);
            (_h->count)--;
        }

        // Inserts data
        pbucket = Tf::smalloc(data.size());
        memcpy(pbucket, data.data(), data.size());
        _h->setBucketPtr(idx, pbucket);
        (_h->count)++;
        break;
    }

    // Rehash
    if (loadFactor() > 0.8) {
        rehash();
    }
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
            int alcsize = Tf::allocsize(pbucket);
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
        Tf::sfree(_h->bucketPtr(idx));
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
        Tf::sfree(_h->bucketPtr(idx));
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
            Tf::sfree(pbucket);
            (_h->count)--;
        }
        _h->setBucketPtr(i, nullptr);
    }
}


void TSharedMemoryHash::rehash()
{
    Bucket bucket;
    QByteArray buf;

    if (loadFactor() < 0.4) {
        // do nothing
        return;
    }

    uint64_t *const oldt = (uint64_t *)_h->hashg();
    int oldsize = _h->tableSize;

    // Creates new table
    _h->tableSize = oldsize * 4;  // new table size
    auto *ptr = Tf::scalloc(_h->tableSize, sizeof(int64_t));
    _h->setHashg(ptr);

    for (int i = 0; i < oldsize; i++) {
        uint64_t g = *(oldt + i);
        void *pbucket = (g && g != -1UL) ? (caddr_t)_h + g : nullptr;

        if (!pbucket) {
            continue;
        }

        int alcsize = Tf::allocsize(pbucket);
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

    Tf::sfree(oldt);
}
