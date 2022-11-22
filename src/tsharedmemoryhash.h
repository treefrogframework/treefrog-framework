#pragma once
#include <TGlobal>

struct hash_header_t;
class Bucket;
class TSharedMemoryAllocator;


class TSharedMemoryHash {
public:
    TSharedMemoryHash(const QString &name, size_t size);
    ~TSharedMemoryHash();

    bool insert(const QByteArray &key, const QByteArray &value);
    QByteArray value(const QByteArray &key, const QByteArray &defaultValue = QByteArray()) const;
    QByteArray take(const QByteArray &key, const QByteArray &defaultValue = QByteArray());
    bool remove(const QByteArray &key);
    uint count() const;
    uint tableSize() const;
    void clear();
    float loadFactor() const;
    void rehash();

protected:
    int find(const QByteArray &key, Bucket &bucket) const;
    int searchIndex(int first);
    uint index(const QByteArray &key) const;
    uint next(uint index) const;

    void lockForRead() const;
    void lockForWrite();
    void unlock() const;

private:
    TSharedMemoryAllocator *_allocator {nullptr};
    hash_header_t *_h {nullptr};

    T_DISABLE_COPY(TSharedMemoryHash)
    T_DISABLE_MOVE(TSharedMemoryHash)
};
