#pragma once
#include <QByteArray>
#include <QStringList>
#include <TGlobal>
#include <TKvsDatabase>
#include <TfNamespace>

class TMemcachedDriver;


class T_CORE_EXPORT TMemcached {
public:
    TMemcached();
    virtual ~TMemcached() { }

    bool isOpen() const;
    QByteArray get(const QByteArray &key, uint *flags = nullptr);
    int64_t getNumber(const QByteArray &key, bool *ok = nullptr, uint *flags = nullptr);
    bool set(const QByteArray &key, const QByteArray &value, int seconds, uint flags = 0);
    bool set(const QByteArray &key, int64_t value, int seconds, uint flags = 0);
    bool add(const QByteArray &key, const QByteArray &value, int seconds, uint flags = 0);
    bool add(const QByteArray &key, int64_t value, int seconds, uint flags = 0);
    bool replace(const QByteArray &key, const QByteArray &value, int seconds, uint flags = 0);
    bool replace(const QByteArray &key, int64_t value, int seconds, uint flags = 0);
    bool append(const QByteArray &key, const QByteArray &value, int seconds, uint flags = 0);
    bool prepend(const QByteArray &key, const QByteArray &value, int seconds, uint flags = 0);
    bool remove(const QByteArray &key);
    uint64_t incr(const QByteArray &key, uint64_t value, bool *ok = nullptr);
    uint64_t decr(const QByteArray &key, uint64_t value, bool *ok = nullptr);
    bool flushAll();
    QByteArray version();

private:
    QByteArray request(const QByteArray &command, const QByteArray &key, const QByteArray &value, uint flags, int exptime, bool noreply);
    QByteArray requestLine(const QByteArray &command, const QByteArray &key, const QByteArray &value, bool noreply);

    TMemcached(Tf::KvsEngine engine);
    TMemcachedDriver *driver();
    const TMemcachedDriver *driver() const;

    TKvsDatabase _database;

    friend class TCacheMemcachedStore;
    T_DISABLE_COPY(TMemcached)
    T_DISABLE_MOVE(TMemcached)
};
