/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcacheredisstore.h"
#include <TRedis>
#include <QDateTime>


TCacheRedisStore::TCacheRedisStore()
{ }


bool TCacheRedisStore::open()
{
    return true;
}


void TCacheRedisStore::close()
{ }


QByteArray TCacheRedisStore::get(const QByteArray &key)
{
    TRedis redis;
    qint64 current = QDateTime::currentMSecsSinceEpoch() / 1000;

return QByteArray();
}


bool TCacheRedisStore::set(const QByteArray &key, const QByteArray &value, int seconds)
{
    TRedis redis;

    qint64 expire = QDateTime::currentMSecsSinceEpoch() / 1000 + seconds;

    return true;
}


bool TCacheRedisStore::remove(const QByteArray &key)
{
    TRedis redis;
return true;
}


void TCacheRedisStore::clear()
{

}


void TCacheRedisStore::gc()
{
    TRedis redis;
    qint64 current = QDateTime::currentMSecsSinceEpoch() / 1000;


}


QMap<QString, QVariant> TCacheRedisStore::defaultSettings() const
{
    return QMap<QString, QVariant>();
}
