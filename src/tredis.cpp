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


QByteArray TRedis::get(const QByteArray &key)
{
    if (!driver()) {
        return QByteArray();
    }

    QVariantList reply;
    QList<QByteArray> command = { "GET", key };
    bool res = driver()->request(command, reply);
    return (res) ? reply.value(0).toByteArray() : QByteArray();
}


bool TRedis::set(const QByteArray &key, const QByteArray &value)
{
    if (!driver()) {
        return false;
    }

    QVariantList reply;
    QList<QByteArray> command = { "SET", key, value };
    return driver()->request(command, reply);
}


bool TRedis::setEx(const QByteArray &key, const QByteArray &value, int seconds)
{
    if (!driver()) {
        return false;
    }

    QVariantList reply;
    QList<QByteArray> command = { "SETEX", key, QByteArray::number(seconds), value };
    return driver()->request(command, reply);
}


QByteArray TRedis::getSet(const QByteArray &key, const QByteArray &value)
{
    if (!driver()) {
        return QByteArray();
    }

    QVariantList reply;
    QList<QByteArray> command = { "GETSET", key, value };
    bool res = driver()->request(command, reply);
    return (res) ? reply.value(0).toByteArray() : QByteArray();
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

    QVariantList reply;
    QList<QByteArray> command = { "DEL" };
    command << keys;
    bool res = driver()->request(command, reply);
    return (res) ? reply.value(0).toInt() : 0;
}
