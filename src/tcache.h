#ifndef TCACHE_H
#define TCACHE_H

#include <TGlobal>
#include <QByteArray>

class TCacheStore;


class T_CORE_EXPORT TCache {
public:
    enum BackEnd {
        SingleFile = 0,
        Redis,
        MongoDB,
    };

    TCache(BackEnd backend, bool lz4Compression = true);
    ~TCache();

    bool open();
    void close();
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
