#ifndef TREDIS_H
#define TREDIS_H

#include <QVariant>
#include <TGlobal>
#include <TKvsDatabase>

class TRedisDriver;


class T_CORE_EXPORT TRedis
{
public:
    TRedis();
    TRedis(const TRedis &other);
    virtual ~TRedis() { }

    bool isOpen() const;
    //bool exists(const QByteArray &key);
    QByteArray get(const QByteArray &key);
    bool set(const QByteArray &key, const QByteArray &value);
    bool setEx(const QByteArray &key, const QByteArray &value, int seconds);
    QByteArray getSet(const QByteArray &key, const QByteArray &value);
    bool del(const QByteArray &key);
    int del(const QList<QByteArray> &keys);

private:
    TRedisDriver *driver();
    const TRedisDriver *driver() const;

private:
    TKvsDatabase database;
};

#endif // TREDIS_H
