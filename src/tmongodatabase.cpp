/* Copyright (c) 2012-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TMongoDatabase>
#include <TMongoDriver>
#include <TSystemGlobal>
#include <QMutex>
#include <QMutexLocker>

#define TF_DEFAULT_CONNECTION  "tf_mongo_default_connection"

static QMap<QString, TMongoDatabase> databaseMap;
static QMutex mutex(QMutex::Recursive);


TMongoDatabase TMongoDatabase::database(const QString &connectionName)
{
    QMutexLocker lock(&mutex);
    QString name = (connectionName.isEmpty()) ? TF_DEFAULT_CONNECTION : connectionName;
    return databaseMap[name];
}


TMongoDatabase TMongoDatabase::addDatabase(const QString &host, const QString &connectionName)
{
    QMutexLocker lock(&mutex);

    TMongoDatabase db;
    db.d = new TMongoDriver();
    db.setHostName(host);
    QString name = (connectionName.isEmpty()) ? TF_DEFAULT_CONNECTION : connectionName;
    databaseMap.insert(name, db);
    return db;
}


void TMongoDatabase::removeDatabase(const QString &connectionName)
{
    QMutexLocker lock(&mutex);

    TMongoDatabase db = database(connectionName);
    db.close();
    delete db.d;

    QString name = (connectionName.isEmpty()) ? TF_DEFAULT_CONNECTION : connectionName;
    databaseMap.remove(name);
}


TMongoDatabase::TMongoDatabase()
    : d(0)
{ }


bool TMongoDatabase::isValid () const
{
    return (bool)d;
}


bool TMongoDatabase::open()
{
    return (driver()) ? driver()->open(host) : false;
}


void TMongoDatabase::close()
{
    if (driver())
        driver()->close();
}


bool TMongoDatabase::isOpen() const
{
    return (driver()) ? driver()->isOpen() : false;
}


TMongoDriver *TMongoDatabase::driver()
{
    return d;
}


const TMongoDriver *TMongoDatabase::driver() const
{
    return d;
}


TMongoDatabase &TMongoDatabase::operator=(const TMongoDatabase &other)
{
    host = other.host;
    d = other.d;
    return *this;
}
