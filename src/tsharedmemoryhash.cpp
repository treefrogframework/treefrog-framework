#include "tsharedmemoryhash.h"
#include "tshm.h"
#include "tmalloc.h"
#include <QDataStream>

#define FREE ((void *)-1)


struct Bucket {
    QByteArray key;
    QByteArray value;

    friend QDataStream &operator<<(QDataStream &ds, const Bucket &bucket);
    friend QDataStream &operator>>(QDataStream &ds, Bucket &bucket);
};

QDataStream &operator<<(QDataStream &ds, const Bucket &bucket)
{
    ds << bucket.key << bucket.value;
    return ds;
}

QDataStream &operator>>(QDataStream &ds, Bucket &bucket)
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
    _h = (hash_header_t *)((caddr_t)ptr + 24);

qDebug() << "hash_header_t *ptr:" << _h;

    if (newmap) {
        ptr = Tf::tmalloc(sizeof(INIT_HEADER));
qDebug() << "tmalloc ptr:" << ptr;

        Q_ASSERT(ptr == _h);
        memcpy(_h, &INIT_HEADER, sizeof(INIT_HEADER));
qDebug() << "_h->tableSize:" << _h->tableSize;
        _h->hashTable = (void **)Tf::tcalloc(_h->tableSize, sizeof(void*));
    }
qDebug() << "---------------";
}


void TSharedMemoryHash::insert(const QByteArray &key, const QByteArray &value)
{
    if (key.isEmpty()) {
        return;
    }

    uint idx = index(key);
qDebug() << "--=====1"  << key << idx;

    QByteArray data;
    QDataStream ds(&data, QIODeviceBase::WriteOnly);
    ds << Bucket{key, value};

    Bucket bucket;
    QByteArray buf;

qDebug() << "--=====2";

    for (;;) {
qDebug() << "--=====3";
        void **pbucket = _h->hashTable + idx;
qDebug() << "--=====3.2" << pbucket << "*pbucket:" << *pbucket;
        if (*pbucket && *pbucket != FREE) {
qDebug() << "--=====4";

            // checks key
            int alcsize = Tf::allocsize(*pbucket);
            if (alcsize <= 0) {
                // error
                Q_ASSERT(0);
                break;
            }

            buf.setRawData((char *)*pbucket, alcsize);
            QDataStream dsb(buf);

            dsb >> bucket;
            if (bucket.key != key) {
qDebug() << "--=====4.3";
                idx = next(idx);
                continue;
            }
        }

        *pbucket = Tf::tmalloc(data.size());
        memcpy(*pbucket, data.data(), data.size());
qDebug() << "--=====5   insert!!!!!!" << "*pbucket:" << *pbucket;
        break;
    }

    (_h->count)++;
qDebug() << "--=====6";

    // Rehash
    if (loadFactor() > 0.8) {
        rehash();
    }
}


void *TSharedMemoryHash::find(const QByteArray &key, Bucket &bucket) const
{
    if (key.isEmpty()) {
        return nullptr;
    }

    uint idx = index(key);
qDebug() << "find --### 0" << key << idx;
    QByteArray buf;

    for (;;) {
qDebug() << "find --### 1";
        void **pbucket = _h->hashTable + idx;
qDebug() << "find --### 2";
        if (!*pbucket) {
qDebug() << "find --### 3";
            break;
        }

        if (*pbucket == FREE) {
qDebug() << "find --### 4";
            continue;
        }

        int alcsize = Tf::allocsize(*pbucket);
        if (alcsize <= 0) {
qDebug() << "find --### 5";
            break;
        }

qDebug() << "find --### 6";
        buf.setRawData((char *)*pbucket, alcsize);
        QDataStream ds(buf);
        ds >> bucket;

qDebug() << "find --### 6.5" << bucket.key;
        if (bucket.key == key) {
qDebug() << "find --### 7  found!!";
            // Found
            return *pbucket;
        }

        idx = next(idx);
    }
    return nullptr;
}


QByteArray TSharedMemoryHash::value(const QByteArray &key, const QByteArray &defaultValue) const
{
    Bucket bucket;
    void *ptr = find(key, bucket);
    return ptr ? bucket.value : defaultValue;
}


QByteArray TSharedMemoryHash::take(const QByteArray &key, const QByteArray &defaultValue)
{
    Bucket bucket;
    void *ptr = find(key, bucket);
    if (ptr) {
        ptr = FREE;
        (_h->count)--;
    }
    return ptr ? bucket.value : defaultValue;
}


bool TSharedMemoryHash::remove(const QByteArray &key)
{
    Bucket bucket;
    void *ptr = find(key, bucket);
    if (ptr) {
        ptr = FREE;
        (_h->count)--;
    }
    return (bool)ptr;
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
    void **oldp = _h->hashTable;

    for (int i = 0; i < _h->tableSize; i++) {
        void *bucketp = oldp[i];
        Tf::tfree(bucketp);
        oldp[i] = nullptr;
    }
}


void TSharedMemoryHash::rehash()
{
    Bucket bucket;
    void **oldt = _h->hashTable;
    int oldsize = _h->tableSize;

    if (loadFactor() < 0.4) {
        // do nothing
        return;
    }

    _h->tableSize = oldsize * 4;  // new table size
    _h->hashTable = (void **)Tf::tcalloc(_h->tableSize, sizeof(void*));

    for (int i = 0; i < oldsize; i++) {
        void **pbucket = oldt + i;
        if (!*pbucket || *pbucket == FREE) {
            continue;
        }

        int alcsize = Tf::allocsize(*pbucket);
        if (alcsize <= 0) {
            continue;
        }

        QByteArray buf = QByteArray::fromRawData((char *)*pbucket, alcsize);
        QDataStream ds(buf);
        ds >> bucket;

        int newidx = index(bucket.key);
        (_h->hashTable)[newidx] = *pbucket;
    }

    Tf::tfree(oldt);
}
