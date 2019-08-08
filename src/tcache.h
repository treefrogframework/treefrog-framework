#ifndef TCACHE_H
#define TCACHE_H

#include <TGlobal>


class T_CORE_EXPORT TCache
{
public:
    ~TCache();

    bool open();
    void close();
    bool set(const QByteArray &key, const QByteArray &value, qint64 msecs);
    QByteArray get(const QByteArray &key);
    void remove(const QByteArray &key);
    void clear();

    QString backend() const;
    static TCache &instance();
    static void setCompressionEnabled(bool enable) { compression = enable; }

private:
    static bool compression;
    int _gcDivisor {0};

    T_DISABLE_COPY(TCache)
    T_DISABLE_MOVE(TCache)
    TCache();
};

#endif // TCACHE_H
