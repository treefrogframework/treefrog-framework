/* Copyright (c) 2012-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tmongodriver.h"
#include "tredisdriver.h"
#include "tmemcacheddriver.h"
#include "tsharedmemorykvsdriver.h"
#include <QMap>
#include <QReadWriteLock>
#include <QString>
#include <TKvsDatabase>
#include <TKvsDriver>
#include <TSystemGlobal>

/*!
  \class TKvsDatabase
  \brief The TKvsDatabase class represents a connection to a key-value
  store database.
*/

const char *const TKvsDatabase::defaultConnection = "tf_default_connection";


// Map of connection name and database data
class TKvsDatabaseDict : public QMap<QString, TKvsDatabaseData> {
public:
    mutable QReadWriteLock lock;
};


static TKvsDatabaseDict *databaseDict()
{
    static TKvsDatabaseDict *dict = new TKvsDatabaseDict;
    return dict;
}


static TKvsDriver *createDriver(const QString &driverName)
{
    TKvsDriver *driver = nullptr;
    QString name = driverName.toLower();

    if (name.isEmpty()) {
        return driver;
    }

    if (name == QLatin1String("mongodb")) {
        driver = new TMongoDriver();
    } else if (name == QLatin1String("redis")) {
        driver = new TRedisDriver();
    } else if (name == QLatin1String("memcached")) {
        driver = new TMemcachedDriver();
    } else if (name == QLatin1String("memory")) {
        driver = new TSharedMemoryKvsDriver();
    } else {
        Tf::warn("TKvsDatabase: {} driver not found", driverName);
        return driver;
    }

    if (!driver) {
        Tf::warn("TKvsDatabase: {} driver not loaded", driverName);
    }
    return driver;
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


TKvsDatabaseData TKvsDatabase::settings(const QString &connectionName)
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[connectionName];
}


TKvsDatabase::TKvsDatabase(const TKvsDatabase &other) :
    connectName(other.connectName),
    drv(other.drv)
{
}


TKvsDatabase::TKvsDatabase(const QString &connectionName, TKvsDriver *driver) :
    connectName(connectionName),
    drv(driver)
{
}


TKvsDatabase::TKvsDatabase(const TKvsDatabaseData &data) :
    connectName(data.connectionName),
    drv(data.driver)
{
}


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


bool TKvsDatabase::command(const QString &cmd)
{
    return (driver()) ? driver()->command(cmd.toUtf8()) : false;
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


QStringList TKvsDatabase::postOpenStatements() const
{
    auto *dict = databaseDict();
    QReadLocker locker(&dict->lock);
    return (*dict)[connectName].postOpenStatements;
}


void TKvsDatabase::setPostOpenStatements(const QStringList &statements)
{
    if (!connectName.isEmpty()) {
        auto *dict = databaseDict();
        QWriteLocker locker(&dict->lock);
        (*dict)[connectName].postOpenStatements = statements;
    }
}


void TKvsDatabase::moveToThread(QThread *targetThread)
{
    if (driver()) {
        driver()->moveToThread(targetThread);
    }
}
