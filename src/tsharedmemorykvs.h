#pragma once
#include <TGlobal>

struct hash_header_t;
class TSharedMemoryAllocator;


class T_CORE_EXPORT TSharedMemoryKvs {
public:
    class Bucket {
    public:
        QByteArray key;
        QByteArray value;
        qint64 expires {0};  // msecs since epoch

        bool isExpired() const
        {
            return expires <= Tf::getMSecsSinceEpoch();
        }

        void clear()
        {
            key.resize(0);
            value.resize(0);
            expires = 0;
        }

        friend QDataStream &operator<<(QDataStream &ds, const Bucket &bucket)
        {
            ds << bucket.key << bucket.value << bucket.expires;
            return ds;
        }

        friend QDataStream &operator>>(QDataStream &ds, Bucket &bucket)
        {
            ds >> bucket.key >> bucket.value >> bucket.expires;
            return ds;
        }
    };

    class WriteLockingIterator {
    public:
        ~WriteLockingIterator();
        const QByteArray &key() const;
        const QByteArray &value() const;
        bool isExpired() const;
        const QByteArray &operator*() const;
        WriteLockingIterator &operator++();
        bool operator==(const WriteLockingIterator &other) const { return _hash == other._hash && _it == other._it; }
        bool operator!=(const WriteLockingIterator &other) const { return _hash != other._hash || _it != other._it; }
        void remove();
    private:
        WriteLockingIterator(TSharedMemoryKvs *hash, uint it);
        void search();

        TSharedMemoryKvs *_hash {nullptr};
        uint _it {0};
        bool _locked {false};
        Bucket _tmpbk;
        friend class TSharedMemoryKvs;
    };

    TSharedMemoryKvs(const QString &name, size_t size = 0);
    ~TSharedMemoryKvs();

    QByteArray get(const QByteArray &key);
    bool set(const QByteArray &key, const QByteArray &value, int seconds);
    bool remove(const QByteArray &key);
    uint count() const;
    uint tableSize() const;
    void clear();
    void gc();
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
    void lockForWrite() const;
    void unlock() const;

private:
    TSharedMemoryAllocator *_allocator {nullptr};
    hash_header_t *_h {nullptr};

    T_DISABLE_COPY(TSharedMemoryKvs)
    T_DISABLE_MOVE(TSharedMemoryKvs)
};
