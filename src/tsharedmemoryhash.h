#pragma once
#include <TGlobal>

struct hash_header_t;
class TSharedMemoryAllocator;


class TSharedMemoryHash {
public:
    class Bucket {
    public:
        QByteArray key;
        QByteArray value;

        void clear()
        {
            key.resize(0);
            value.resize(0);
        }
        friend QDataStream &operator<<(QDataStream &ds, const Bucket &bucket)
        {
            ds << bucket.key << bucket.value;
            return ds;
        }
        friend QDataStream &operator>>(QDataStream &ds, Bucket &bucket)
        {
            ds >> bucket.key >> bucket.value;
            return ds;
        }
    };

    class WriteLockingIterator {
    public:
        ~WriteLockingIterator();
        const QByteArray &key() const;
        const QByteArray &value() const;
        const QByteArray &operator*() const;
        WriteLockingIterator &operator++();
        bool operator==(const WriteLockingIterator &other) const { return _hash == other._hash && _it == other._it; }
        bool operator!=(const WriteLockingIterator &other) const { return _hash != other._hash || _it != other._it; }
        void remove();
    private:
        WriteLockingIterator(TSharedMemoryHash *hash, uint it);
        void search();

        TSharedMemoryHash *_hash {nullptr};
        uint _it {0};
        bool _locked {false};
        Bucket _tmpbk;
        friend class TSharedMemoryHash;
    };

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

    WriteLockingIterator begin();
    WriteLockingIterator end();

protected:
    uint find(const QByteArray &key, Bucket &bucket) const;
    bool find(uint index, Bucket &bucket) const;
    int searchIndex(int first);
    uint index(const QByteArray &key) const;
    uint next(uint index) const;
    void remove(uint index);

    void lockForRead() const;
    void lockForWrite();
    void unlock() const;

private:
    TSharedMemoryAllocator *_allocator {nullptr};
    hash_header_t *_h {nullptr};

    T_DISABLE_COPY(TSharedMemoryHash)
    T_DISABLE_MOVE(TSharedMemoryHash)
};
