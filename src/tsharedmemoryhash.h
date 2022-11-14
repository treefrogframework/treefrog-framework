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
    int find(const QByteArray &key, Bucket &bucket) const;
    int searchIndex(int first);
    uint index(const QByteArray &key) const;
    uint next(uint index) const;

private:
    struct hash_header_t {
        uint hashtg {0};
        int tableSize {1024};
        int count {0};

        uint64_t *hashg() { return (uint64_t *)((caddr_t)this + hashtg); }

        void setHashg(void *p)
        {
            hashtg = (uint64_t)p - (uint64_t)this;
        }

        void *bucketPtr(int index)
        {
            if (index >= 0 && index < tableSize) {
                uint64_t g = *(hashg() + index);
                return (g && g != -1UL) ? (caddr_t)this + g : (void *)g;
            } else {
                Q_ASSERT(0);
            }
            return nullptr;
        }

        void setBucketPtr(int index, void *ptr)
        {
            if (index >= 0 && index < tableSize) {
                *(hashg() + index) = (ptr && ptr != (void *)-1) ? (uint64_t)ptr - (uint64_t)this : (uint64_t)ptr;
            } else {
                Q_ASSERT(0);
            }
        }
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
