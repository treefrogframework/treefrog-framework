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
    QString get(const QByteArray &key, uint *flags = nullptr);
    qint64 getNumber(const QByteArray &key, uint *flags = nullptr, bool *ok = nullptr);
    bool set(const QByteArray &key, const QString &value, int seconds, uint flags = 0);
    bool set(const QByteArray &key, qint64 value, int seconds, uint flags = 0);
    bool add(const QByteArray &key, const QString &value, int seconds, uint flags = 0);
    bool add(const QByteArray &key, qint64 value, int seconds, uint flags = 0);
    bool replace(const QByteArray &key, const QString &value, int seconds, uint flags = 0);
    bool replace(const QByteArray &key, qint64 value, int seconds, uint flags = 0);
    bool append(const QByteArray &key, const QString &value, int seconds, uint flags = 0);
    bool prepend(const QByteArray &key, const QString &value, int seconds, uint flags = 0);
    bool remove(const QByteArray &key);
    quint64 incr(const QByteArray &key, quint64 value, bool *ok = nullptr);
    quint64 decr(const QByteArray &key, quint64 value, bool *ok = nullptr);
    QByteArray version();

private:
    QByteArray request(const QByteArray &command, const QByteArray &key, const QByteArray &value, uint flags, int exptime, bool noreply);
    QByteArray requestLine(const QByteArray &command, const QByteArray &key, const QByteArray &value, bool noreply);

    TMemcached(Tf::KvsEngine engine);
    TMemcachedDriver *driver();
    const TMemcachedDriver *driver() const;

    TKvsDatabase _database;

    T_DISABLE_COPY(TMemcached)
    T_DISABLE_MOVE(TMemcached)
};
