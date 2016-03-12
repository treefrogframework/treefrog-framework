/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TRedis>
#include <TActionContext>
#include "tredisdriver.h"

/*!
  \class TRedis
  \brief The TRedis class provides a means of operating a Redis
  system.
*/

/*!
  Constructs a TRedis object.
*/
TRedis::TRedis()
    : database(Tf::currentContext()->getKvsDatabase(TKvsDatabase::Redis))
{ }

/*!
  Copy constructor.
*/
TRedis::TRedis(const TRedis &other)
    : database(other.database)
{ }

/*!
  Returns the MongoDB driver associated with the TRedis object.
*/
TRedisDriver *TRedis::driver()
{
#ifdef TF_NO_DEBUG
    return (TRedisDriver *)database.driver();
#else
    if (!database.driver()) {
        return nullptr;
    }

    TRedisDriver *driver = dynamic_cast<TRedisDriver *>(database.driver());
    if (!driver) {
        throw RuntimeException("cast error", __FILE__, __LINE__);
    }
    return driver;
#endif
}

/*!
  Returns the MongoDB driver associated with the TRedis object.
*/
const TRedisDriver *TRedis::driver() const
{
#ifdef TF_NO_DEBUG
    return (const TRedisDriver *)database.driver();
#else
    if (!database.driver()) {
        return nullptr;
    }

    const TRedisDriver *driver = dynamic_cast<const TRedisDriver *>(database.driver());
    if (!driver) {
        throw RuntimeException("cast error", __FILE__, __LINE__);
    }
    return driver;
#endif
}


bool TRedis::isOpen() const
{
    return (driver()) ? driver()->isOpen() : false;
}


bool TRedis::exists(const QByteArray &key)
{
    if (!driver()) {
        return false;
    }

    QVariantList resp;
    QList<QByteArray> command = { "EXISTS", key };
    bool res = driver()->request(command, resp);
    return (res && resp.value(0).toInt() == 1);
}


QByteArray TRedis::get(const QByteArray &key)
{
    if (!driver()) {
        return QByteArray();
    }

    QVariantList resp;
    QList<QByteArray> command = { "GET", key };
    bool res = driver()->request(command, resp);
    return (res) ? resp.value(0).toByteArray() : QByteArray();
}


bool TRedis::set(const QByteArray &key, const QByteArray &value)
{
    if (!driver()) {
        return false;
    }

    QVariantList resp;
    QList<QByteArray> command = { "SET", key, value };
    return driver()->request(command, resp);
}


bool TRedis::setEx(const QByteArray &key, const QByteArray &value, int seconds)
{
    if (!driver()) {
        return false;
    }

    QVariantList resp;
    QList<QByteArray> command = { "SETEX", key, QByteArray::number(seconds), value };
    return driver()->request(command, resp);
}


QByteArray TRedis::getSet(const QByteArray &key, const QByteArray &value)
{
    if (!driver()) {
        return QByteArray();
    }

    QVariantList resp;
    QList<QByteArray> command = { "GETSET", key, value };
    bool res = driver()->request(command, resp);
    return (res) ? resp.value(0).toByteArray() : QByteArray();
}


bool TRedis::del(const QByteArray &key)
{
    QList<QByteArray> keys = { key };
    int count = del(keys);
    return (count == 1);
}


int TRedis::del(const QList<QByteArray> &keys)
{
    if (!driver()) {
        return 0;
    }

    QVariantList resp;
    QList<QByteArray> command = { "DEL" };
    command << keys;
    bool res = driver()->request(command, resp);
    return (res) ? resp.value(0).toInt() : 0;
}

/*!
  Inserts all the \a values at the tail of the list stored at key.
  Returns the length of the list after the push operation.
 */
int TRedis::rpush(const QByteArray &key, const QList<QByteArray> &values)
{
    if (!driver()) {
        return false;
    }

    QVariantList resp;
    QList<QByteArray> command = { "RPUSH", key };
    command << values;
    bool res = driver()->request(command, resp);
    return (res) ? resp.value(0).toInt() : 0;
}

/*!
  Inserts all the \a values at the tail of the list stored at key.
  Returns the length of the list after the push operation.
 */
int TRedis::lpush(const QByteArray &key, const QList<QByteArray> &values)
{
    if (!driver()) {
        return false;
    }

    QVariantList resp;
    QList<QByteArray> command = { "LPUSH", key };
    command << values;
    bool res = driver()->request(command, resp);
    return (res) ? resp.value(0).toInt() : 0;
}


QList<QByteArray> TRedis::lrange(const QByteArray &key, int start, int end)
{
    if (!driver()) {
        return QList<QByteArray>();
    }

    QList<QByteArray> ret;
    QVariantList resp;
    QList<QByteArray> command = { "LRANGE", key, QByteArray::number(start), QByteArray::number(end) };
    bool res = driver()->request(command, resp);
    if (res) {
        for (auto &var : resp) {
            ret << var.toByteArray();
        }
    }
    return ret;
}


QByteArray TRedis::lindex(const QByteArray &key, int index)
{
    if (!driver()) {
        return QByteArray();
    }

    QVariantList resp;
    QList<QByteArray> command = { "LINDEX", key, QByteArray::number(index) };
    bool res = driver()->request(command, resp);
    return (res) ? resp.value(0).toByteArray() : QByteArray();
}

/*!
  Returns the length of the list stored at key.
*/
int TRedis::llen(const QByteArray &key)
{
    if (!driver()) {
        return -1;
    }

    QVariantList resp;
    QList<QByteArray> command = { "LLEN", key };
    bool res = driver()->request(command, resp);
    return (res) ? resp.value(0).toInt() : -1;
}


QList<QByteArray> TRedis::toByteArrayList(const QStringList &values)
{
    QList<QByteArray> ret;
    for (auto &val : values) {
        ret << val.toUtf8();
    }
    return ret;
}


QStringList TRedis::toStringList(const QList<QByteArray> &values)
{
    QStringList ret;
    for (auto &val : values) {
        ret << QString::fromUtf8(val);
    }
    return ret;
}
