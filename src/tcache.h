#ifndef TCACHE_H
#define TCACHE_H

#include <TGlobal>
#include <QByteArray>

class TSQLiteBlobStore;


class T_CORE_EXPORT TCache {
public:
    TCache(qint64 thresholdFileSize, int gcDivisor = 100);
    ~TCache();

    bool insert(const QByteArray &key, const QByteArray &value, int seconds);
    QByteArray value(const QByteArray &key, const QByteArray &defaultValue = QByteArray());
    QByteArray take(const QByteArray &key);
    void remove(const QByteArray &key);
    int count() const;
    void clear();

    static bool setup();

private:
    qint64 fileSize() const;

    qint64 _thresholdFileSize {0};
    int _gcDivisor {0};
    TSQLiteBlobStore *_blobStore {nullptr};
};

#endif // TCACHE_H
