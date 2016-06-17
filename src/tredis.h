#ifndef TREDIS_H
#define TREDIS_H

#include <QVariant>
#include <QByteArray>
#include <QStringList>
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
    bool exists(const QByteArray &key);

    // binary
    QByteArray get(const QByteArray &key);
    bool set(const QByteArray &key, const QByteArray &value);
    bool setEx(const QByteArray &key, const QByteArray &value, int seconds);
    QByteArray getSet(const QByteArray &key, const QByteArray &value);

    // string
    QString gets(const QByteArray &key);
    bool sets(const QByteArray &key, const QString &value);
    bool setsEx(const QByteArray &key, const QString &value, int seconds);
    QString getsSets(const QByteArray &key, const QString &value);

    bool del(const QByteArray &key);
    int del(const QList<QByteArray> &keys);

    // binary list
    int rpush(const QByteArray &key, const QList<QByteArray> &values);
    int lpush(const QByteArray &key, const QList<QByteArray> &values);
    QList<QByteArray> lrange(const QByteArray &key, int start, int end);
    QByteArray lindex(const QByteArray &key, int index);

    // string list
    int rpushs(const QByteArray &key, const QStringList &values);
    int lpushs(const QByteArray &key, const QStringList &values);
    QStringList lranges(const QByteArray &key, int start, int end);
    QString lindexs(const QByteArray &key, int index);

    int llen(const QByteArray &key);

private:
    TRedisDriver *driver();
    const TRedisDriver *driver() const;

    static QList<QByteArray> toByteArrayList(const QStringList &values);
    static QStringList toStringList(const QList<QByteArray> &values);

    TKvsDatabase database;
};


inline QString TRedis::gets(const QByteArray &key)
{
    return QString::fromUtf8(get(key));
}

inline bool TRedis::sets(const QByteArray &key, const QString &value)
{
    return set(key, value.toUtf8());
}

inline bool TRedis::setsEx(const QByteArray &key, const QString &value, int seconds)
{
    return setEx(key, value.toUtf8(), seconds);
}

inline QString TRedis::getsSets(const QByteArray &key, const QString &value)
{
    return QString::fromUtf8(getSet(key, value.toUtf8()));
}

inline int TRedis::rpushs(const QByteArray &key, const QStringList &values)
{
    return rpush(key, toByteArrayList(values));
}

inline int TRedis::lpushs(const QByteArray &key, const QStringList &values)
{
    return lpush(key, toByteArrayList(values));
}

inline QStringList TRedis::lranges(const QByteArray &key, int start, int end = -1)
{
    return toStringList(lrange(key, start, end));
}

inline QString TRedis::lindexs(const QByteArray &key, int index)
{
    return QString::fromUtf8(lindex(key, index));
}

#endif // TREDIS_H
