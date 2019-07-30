#ifndef TCACHE_H
#define TCACHE_H

#include <TGlobal>
#include <QByteArray>

class TCacheStore;


class T_CORE_EXPORT TCache {
public:
    enum CacheType {
        File = 0,
        Redis,
    };

    TCache(CacheType type, bool lz4Compression = true, int gcDivisor = 100);
    ~TCache();

    bool set(const QByteArray &key, const QByteArray &value, qint64 msecs);
    QByteArray get(const QByteArray &key);
    void remove(const QByteArray &key);
    void clear();

private:
    bool _compression {true};
    int _gcDivisor {0};
    TCacheStore *_cacheStore {nullptr};
};

#endif // TCACHE_H
