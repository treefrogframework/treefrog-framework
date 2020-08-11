#pragma once
#include <QByteArray>
#include <QStringList>
#include <QVariant>
#include <TGlobal>
#include <TKvsDatabase>
#include <TfNamespace>

class TRedisDriver;


class T_CORE_EXPORT TRedis {
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
    bool setNx(const QByteArray &key, const QByteArray &value);
    QByteArray getSet(const QByteArray &key, const QByteArray &value);

    // string
    QString gets(const QByteArray &key);
    bool sets(const QByteArray &key, const QString &value);
    bool setsEx(const QByteArray &key, const QString &value, int seconds);
    bool setsNx(const QByteArray &key, const QString &value);
    QString getsSets(const QByteArray &key, const QString &value);

    bool del(const QByteArray &key);
    int del(const QByteArrayList &keys);

    // binary list
    int rpush(const QByteArray &key, const QByteArrayList &values);
    int lpush(const QByteArray &key, const QByteArrayList &values);
    QByteArrayList lrange(const QByteArray &key, int start, int end);
    QByteArray lindex(const QByteArray &key, int index);

    // string list
    int rpushs(const QByteArray &key, const QStringList &values);
    int lpushs(const QByteArray &key, const QStringList &values);
    QStringList lranges(const QByteArray &key, int start, int end);
    QString lindexs(const QByteArray &key, int index);
    int llen(const QByteArray &key);

    // hash
    bool hset(const QByteArray &key, const QByteArray &field, const QByteArray &value);
    bool hsetNx(const QByteArray &key, const QByteArray &field, const QByteArray &value);
    bool hsets(const QByteArray &key, const QByteArray &field, const QString &value);
    bool hsetsNx(const QByteArray &key, const QByteArray &field, const QString &value);
    QByteArray hget(const QByteArray &key, const QByteArray &field);
    QString hgets(const QByteArray &key, const QByteArray &field);
    bool hexists(const QByteArray &key, const QByteArray &field);
    bool hdel(const QByteArray &key, const QByteArray &field);
    int hdel(const QByteArray &key, const QByteArrayList &fields);
    int hlen(const QByteArray &key);
    QList<QPair<QByteArray, QByteArray>> hgetAll(const QByteArray &key);

    void flushDb();

private:
    TRedis(Tf::KvsEngine engine);
    TRedisDriver *driver();
    const TRedisDriver *driver() const;

    static QByteArrayList toByteArrayList(const QStringList &values);
    static QStringList toStringList(const QByteArrayList &values);

    TKvsDatabase database;

    friend class TCacheRedisStore;
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

inline bool TRedis::setsNx(const QByteArray &key, const QString &value)
{
    return setNx(key, value.toUtf8());
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

inline bool TRedis::hsets(const QByteArray &key, const QByteArray &field, const QString &value)
{
    return hset(key, field, value.toUtf8());
}

inline bool TRedis::hsetsNx(const QByteArray &key, const QByteArray &field, const QString &value)
{
    return hsetNx(key, field, value.toUtf8());
}

inline QString TRedis::hgets(const QByteArray &key, const QByteArray &field)
{
    return QString::fromUtf8(hget(key, field));
}

