#pragma once
#include <TGlobal>

struct hash_header_t;
struct Bucket;


class TSharedMemoryHash {
public:
    TSharedMemoryHash(const QString &name, size_t size);

    void insert(const QByteArray &key, const QByteArray &value);
    QByteArray value(const QByteArray &key, const QByteArray &defaultValue = QByteArray()) const;
    QByteArray take(const QByteArray &key, const QByteArray &defaultValue = QByteArray());
    bool remove(const QByteArray &key);
    int	count() const;
    void clear();
    float loadFactor() const;
    void rehash();

protected:
    void *find(const QByteArray &key, Bucket &bucket) const;
    uint index(const QByteArray &key) const;
    uint next(uint index) const;

private:
    struct hash_header_t {
        void **hashTable {nullptr};
        int tableSize {1024};
        int count {0};
    } *_h {nullptr};

    T_DISABLE_COPY(TSharedMemoryHash)
    T_DISABLE_MOVE(TSharedMemoryHash)
};


inline uint TSharedMemoryHash::index(const QByteArray &key) const
{
    return qHash(key) % _h->tableSize;
}


inline uint TSharedMemoryHash::next(uint index) const
{
    return (index + 1) % _h->tableSize;
}
