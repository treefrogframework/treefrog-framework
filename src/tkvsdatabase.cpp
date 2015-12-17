/* Copyright (c) 2012-2015, AOYAMA Kazuharu
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
#include <QStringListIterator>
#include <QMutex>
#include <QMutexLocker>
#include "tmongodriver.h"
#include "tredisdriver.h"

class TKvsDatabaseData
{
public:
    QString connectionName;
    QString databaseName;
    QString hostName;
    quint16 port;
    QString userName;
    QString password;
    QString connectOptions;
    TKvsDriver *driver;  // pointer to a singleton object

    TKvsDatabaseData() : port(0), driver(0) { }
};


/*!
  \class TKvsDatabase
  \brief The TKvsDatabase class represents a connection to a key-value
  store database.
*/

const char *const TKvsDatabase::defaultConnection = "tf_default_connection";

static QMap<QString, TKvsDatabaseData> databaseMap;
static QMutex mutex(QMutex::Recursive);


static TKvsDriver *createDriver(const QString &driverName)
{
    TKvsDriver *ret = 0;

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
    QMutexLocker lock(&mutex);
    TKvsDatabaseData &d = databaseMap[connectionName];
    return TKvsDatabase(d.connectionName, d.driver);
}


TKvsDatabase TKvsDatabase::addDatabase(const QString &driver, const QString &connectionName)
{
    QMutexLocker lock(&mutex);

    // Removes it if exists
    if (databaseMap.contains(connectionName)) {
        removeDatabase(connectionName);
    }
    TKvsDatabaseData data;
    data.connectionName = connectionName;
    data.driver = createDriver(driver);  // creates a driver
    databaseMap.insert(connectionName, data);
    return database(connectionName);
}


void TKvsDatabase::removeDatabase(const QString &connectionName)
{
    QMutexLocker lock(&mutex);
    TKvsDatabase db = database(connectionName);

    db.close();
    if (db.drv) {
        delete db.drv;
    }
    databaseMap.remove(connectionName);
}


void TKvsDatabase::removeAllDatabases()
{
    QMutexLocker lock(&mutex);
    QStringList keys = databaseMap.keys();

    for (QStringListIterator it(keys); it.hasNext(); ) {
        removeDatabase(it.next());
    }

    databaseMap.clear();
}


TKvsDatabase::TKvsDatabase()
    : connectName(), drv(0)
{ }


TKvsDatabase::TKvsDatabase(const TKvsDatabase &other)
    : connectName(other.connectName), drv(other.drv)
{ }


TKvsDatabase::TKvsDatabase(const QString &connectionName, TKvsDriver *driver)
    : connectName(connectionName), drv(driver)
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
    return (driver()) ? driver()->open(databaseName(), userName(), password(), hostName(), port(), connectOptions()) : false;
}


void TKvsDatabase::close()
{
    if (driver())
        driver()->close();
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
    return databaseMap[connectName].databaseName;
}


void TKvsDatabase::setDatabaseName(const QString &name)
{
    if (!connectName.isEmpty()) {
        databaseMap[connectName].databaseName = name;
    }
}


QString TKvsDatabase::hostName() const
{
    return databaseMap[connectName].hostName;
}


void TKvsDatabase::setHostName(const QString &hostName)
{
    if (!connectName.isEmpty()) {
        databaseMap[connectName].hostName = hostName;
    }
}


int TKvsDatabase::port() const
{
    return databaseMap[connectName].port;
}


void TKvsDatabase::setPort(int port)
{
    if (!connectName.isEmpty()) {
        databaseMap[connectName].port = port;
    }
}


QString TKvsDatabase::userName() const
{
    return databaseMap[connectName].userName;
}


void TKvsDatabase::setUserName(const QString &userName)
{
    if (!connectName.isEmpty()) {
        databaseMap[connectName].userName = userName;
    }
}


QString TKvsDatabase::password() const
{
    return databaseMap[connectName].password;
}


void TKvsDatabase::setPassword(const QString &password)
{
    if (!connectName.isEmpty()) {
        databaseMap[connectName].password = password;
    }
}


QString TKvsDatabase::connectOptions() const
{
    return databaseMap[connectName].connectOptions;
}


void TKvsDatabase::setConnectOptions(const QString &options)
{
    if (!connectName.isEmpty()) {
        databaseMap[connectName].connectOptions = options;
    }
}
