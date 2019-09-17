/* Copyright (c) 2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tcachemongostore.h"
#include <TMongoQuery>
#include <QDateTime>


TCacheMongoStore::TCacheMongoStore()
{ }


bool TCacheMongoStore::open()
{
    return true;
}


void TCacheMongoStore::close()
{ }


QByteArray TCacheMongoStore::get(const QByteArray &key)
{
    TMongoQuery mongo("cache");
    qint64 current = QDateTime::currentMSecsSinceEpoch() / 1000;

    QVariantMap cri {{"k", QString(key)}};
    QVariantMap doc = mongo.findOne(cri);
    qint64 expire = doc.value("t").toLongLong();

    if (expire <= current) {
        remove(key);
        return QByteArray();
    }
    return doc.value("v").toByteArray();
}


bool TCacheMongoStore::set(const QByteArray &key, const QByteArray &value, int seconds)
{
    TMongoQuery mongo("cache");

    qint64 expire = QDateTime::currentMSecsSinceEpoch() / 1000 + seconds;
    QVariantMap doc {{"k", QString(key)}, {"v", value}, {"t", expire}};
    QVariantMap cri {{"k", QString(key)}};
    return mongo.update(cri, doc, true);
}


bool TCacheMongoStore::remove(const QByteArray &key)
{
    TMongoQuery mongo("cache");
    QVariantMap cri {{"k", QString(key)}};
    return mongo.remove(cri);
}


void TCacheMongoStore::clear()
{
    TMongoQuery mongo("cache");
    QVariantMap cri;
    mongo.remove(cri);
}


void TCacheMongoStore::gc()
{
    TMongoQuery mongo("cache");
    qint64 current = QDateTime::currentMSecsSinceEpoch() / 1000;

    QVariantMap lte {{"$lte", current}};
    QVariantMap cri {{"t", lte}};
    mongo.remove(cri);
}


QMap<QString, QVariant> TCacheMongoStore::defaultSettings() const
{
    return QMap<QString, QVariant>();
}
