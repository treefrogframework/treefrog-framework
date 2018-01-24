/* Copyright (c) 2012-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TKvsDatabase>
#include <TKvsDriver>
#include <TSystemGlobal>
#include <QMap>
#include <QString>
#include <QReadWriteLock>
#include "tmongodriver.h"
#include "tredisdriver.h"

class TKvsDatabaseData
{
public:
    QString connectionName;
    QString databaseName;
    QString hostName;
    quint16 port {0};
    QString userName;
    QString password;
    QString connectOptions;
    TKvsDriver *driver {nullptr};  // pointer to a singleton object

    TKvsDatabaseData() {}
};


/*!
  \class TKvsDatabase
  \brief The TKvsDatabase class represents a connection to a key-value
  store database.
*/

const char *const TKvsDatabase::defaultConnection = "tf_default_connection";


class TKvsDatabaseDict : public QMap<QString, TKvsDatabaseData>
{
public:
    mutable QReadWriteLock lock;
};
Q_GLOBAL_STATIC(TKvsDatabaseDict, databaseDict)


static TKvsDriver *createDriver(const QString &driverName)
{
    TKvsDriver *ret = nullptr;

    if (driverName == QLatin1String("MONGODB")) {
        ret = new TMongoDriver();
    } else if (driverName == QLatin1String("REDIS")) {
        ret = new TRedisDriver();
    }

    if (!ret) {
        tWarn("TKvsDatabase: %s driver not loaded", qPrintable(driverName));
    }
    return ret;
}


TKvsDatabase TKvsDatabase::database(const QString &connectionName)
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);

    const TKvsDatabaseData &d = (*dict)[connectionName];
    return TKvsDatabase(d.connectionName, d.driver);
}


TKvsDatabase TKvsDatabase::addDatabase(const QString &driver, const QString &connectionName)
{
    auto *dict = databaseDict();
    QWriteLocker locker(&dict->lock);

    // Removes it if exists
    if (dict->contains(connectionName)) {
        auto data = dict->take(connectionName);
        delete data.driver;
    }

    TKvsDatabaseData data;
    data.connectionName = connectionName;
    data.driver = createDriver(driver);  // creates a driver
    dict->insert(connectionName, data);
    return TKvsDatabase(data);
}


void TKvsDatabase::removeDatabase(const QString &connectionName)
{
    auto *dict = databaseDict();
    QWriteLocker locker(&dict->lock);

    TKvsDatabase db(dict->take(connectionName));

    db.close();
    delete db.drv;
}


// void TKvsDatabase::removeAllDatabases()
// {
//     QMutexLocker lock(&mutex);
//     const QStringList keys = databaseDict()->keys();

//     for (auto &key : keys) {
//         removeDatabase(key);
//     }
//     databaseDict()->clear();
// }


TKvsDatabase::TKvsDatabase(const TKvsDatabase &other)
    : connectName(other.connectName), drv(other.drv)
{ }


TKvsDatabase::TKvsDatabase(const QString &connectionName, TKvsDriver *driver)
    : connectName(connectionName), drv(driver)
{ }


TKvsDatabase::TKvsDatabase(const TKvsDatabaseData &data)
    : connectName(data.connectionName), drv(data.driver)
{ }


TKvsDatabase &TKvsDatabase::operator=(const TKvsDatabase &other)
{
    connectName = other.connectName;
    drv = other.drv;
    return *this;
}


bool TKvsDatabase::isValid() const
{
    return (bool)driver();
}


bool TKvsDatabase::open()
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    const TKvsDatabaseData &data = (*dict)[connectName];
    return (driver()) ? driver()->open(data.databaseName, data.userName, data.password, data.hostName, data.port, data.connectOptions) : false;
}


void TKvsDatabase::close()
{
    if (driver()) {
        driver()->close();
    }
}


bool TKvsDatabase::isOpen() const
{
    return (driver()) ? driver()->isOpen() : false;
}


QString TKvsDatabase::driverName() const
{
    return (driver()) ? driver()->key() : QString();
}


QString TKvsDatabase::databaseName() const
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[connectName].databaseName;
}


void TKvsDatabase::setDatabaseName(const QString &name)
{
    if (!connectName.isEmpty()) {
        auto *dict = databaseDict();
        QWriteLocker locker(&dict->lock);
        (*dict)[connectName].databaseName = name;
    }
}


QString TKvsDatabase::hostName() const
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[connectName].hostName;
}


void TKvsDatabase::setHostName(const QString &hostName)
{
    if (!connectName.isEmpty()) {
        auto *dict = databaseDict();
        QWriteLocker locker(&dict->lock);
        (*dict)[connectName].hostName = hostName;
    }
}


int TKvsDatabase::port() const
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[connectName].port;
}


void TKvsDatabase::setPort(int port)
{
    if (!connectName.isEmpty()) {
        auto *dict = databaseDict();
        QWriteLocker locker(&dict->lock);
        (*dict)[connectName].port = port;
    }
}


QString TKvsDatabase::userName() const
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[connectName].userName;
}


void TKvsDatabase::setUserName(const QString &userName)
{
    if (!connectName.isEmpty()) {
        auto *dict = databaseDict();
        QWriteLocker locker(&dict->lock);
        (*dict)[connectName].userName = userName;
    }
}


QString TKvsDatabase::password() const
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[connectName].password;
}


void TKvsDatabase::setPassword(const QString &password)
{
    if (!connectName.isEmpty()) {
        auto *dict = databaseDict();
        QWriteLocker locker(&dict->lock);
        (*dict)[connectName].password = password;
    }
}


QString TKvsDatabase::connectOptions() const
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[connectName].connectOptions;
}


void TKvsDatabase::setConnectOptions(const QString &options)
{
    if (!connectName.isEmpty()) {
        auto *dict = databaseDict();
        QWriteLocker locker(&dict->lock);
        (*dict)[connectName].connectOptions = options;
    }
}
