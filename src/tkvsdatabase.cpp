/* Copyright (c) 2012-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TKvsDatabase>
#include <TKvsDriver>
#include <TMongoDriver>
#include <TSystemGlobal>
#include <QMutex>
#include <QMutexLocker>

const char *const defaultConnection = "tf_default_connection";

static QMap<QString, TKvsDatabase> databaseMap;
static QMutex mutex(QMutex::Recursive);


TKvsDatabase TKvsDatabase::database(const QString &connectionName)
{
    QMutexLocker lock(&mutex);
    return databaseMap[connectionName];
}


TKvsDatabase TKvsDatabase::addDatabase(TKvsDatabase &db, const QString &connectionName)
{
    QMutexLocker lock(&mutex);

    // If it exists..
    if (databaseMap.contains(connectionName))
        removeDatabase(connectionName);

    db.connectName = connectionName;
    databaseMap.insert(connectionName, db);
    return db;
}


TKvsDatabase TKvsDatabase::addDatabase(const QString &driver, const QString &connectionName)
{
    TKvsDatabase db;
    db.createDriver(driver);  // creates the driver
    return addDatabase(db, connectionName);
}


void TKvsDatabase::removeDatabase(const QString &connectionName)
{
    QMutexLocker lock(&mutex);

    TKvsDatabase db = database(connectionName);
    db.close();
    if (db.drv)
        delete db.drv;

    databaseMap.remove(connectionName);
}


void TKvsDatabase::removeAllDatabases()
{
    QMutexLocker lock(&mutex);

    for (QMap<QString, TKvsDatabase>::iterator it = databaseMap.begin(); it != databaseMap.end(); ++it) {
        TKvsDatabase &db = it.value();
        db.close();
        if (db.drv)
            delete db.drv;
    }
    databaseMap.clear();
}


TKvsDatabase::TKvsDatabase()
    : connectName(), dbName(), host(), portNumber(0), user(), pass(), opts(), drvName(), drv(0)
{ }


TKvsDatabase::TKvsDatabase(const TKvsDatabase &other)
    : connectName(other.connectName), dbName(other.dbName), host(other.host),
      portNumber(other.portNumber), user(other.user), pass(other.pass), opts(other.opts),
      drvName(other.drvName), drv(other.drv)
{ }


TKvsDatabase &TKvsDatabase::operator=(const TKvsDatabase &other)
{
    connectName = other.connectName;
    dbName = other.dbName;
    host = other.host;
    portNumber = other.portNumber;
    user = other.user;
    pass = other.pass;
    opts = other.opts;
    drvName = other.drvName;
    drv = other.drv;
    return *this;
}


void TKvsDatabase::createDriver(const QString &driverName)
{
    if (driverName == QLatin1String("MONGODB")) {
        drv = new TMongoDriver();
    }

    if (!drv) {
        tWarn("QSqlDatabase: %s driver not loaded", qPrintable(driverName));
        return;
    }
    drvName = driverName;
}


bool TKvsDatabase::isValid () const
{
    return (bool)driver();
}


bool TKvsDatabase::open()
{
    return (driver()) ? driver()->open(host, portNumber) : false;
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
