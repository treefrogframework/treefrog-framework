/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMutexLocker>
#include <QDateTime>
#include <QHash>
#include <TKvsDatabasePool>
#include <TWebApplication>
#include <TSqlDatabasePool>
#include "tsystemglobal.h"

static TKvsDatabasePool *databasePool = 0;

typedef QHash<QString, int> KvsTypeHash;
Q_GLOBAL_STATIC_WITH_INITIALIZER(KvsTypeHash, kvsTypeHash,
{
    x->insert("MONGODB", TKvsDatabasePool::MongoDB);
});


static void cleanup()
{
    if (databasePool) {
        delete databasePool;
        databasePool = 0;
    }
}


TKvsDatabasePool::~TKvsDatabasePool()
{
    timer.stop();

    QMutexLocker locker(&mutex);
    for (int j = 0; j < pooledConnections.count(); ++j) {
        QMap<QString, QDateTime> &map = pooledConnections[j];
        QMap<QString, QDateTime>::iterator it = map.begin();
        while (it != map.end()) {
            TKvsDatabase::database(it.key()).close();
            it = map.erase(it);
        }
    }

    TKvsDatabase::removeAllDatabases();
}


TKvsDatabasePool::TKvsDatabasePool(const QString &environment)
    : QObject(), dbEnvironment(environment)
{
    // Starts the timer to close extra-connection
    timer.start(10000, this);
}


void TKvsDatabasePool::init()
{
    // Adds databases previously

    for (QHashIterator<QString, int> it(*kvsTypeHash()); it.hasNext(); ) {
        const QString &drv = it.next().key();
        for (int i = 0; i < maxConnectionsPerProcess(); ++i) {
            TKvsDatabase db = TKvsDatabase::addDatabase(drv, QString().sprintf("%s_%d", qPrintable(drv), i));
            if (!db.isValid()) {
                tWarn("KVS init parameter is invalid");
                break;
            }
            tSystemDebug("Add KVS successfully. name:%s", qPrintable(db.connectionName()));
        }

        pooledConnections.append(QMap<QString, QDateTime>());
    }
}


QSettings &TKvsDatabasePool::kvsSettings(TKvsDatabasePool::KvsType type) const
{
    switch (type) {
    case MongoDB:
        return Tf::app()->mongoDbSettings();
        break;
    default:
        break;
    }
    throw RuntimeException("No such KVS type", __FILE__, __LINE__);
}


TKvsDatabase TKvsDatabasePool::pop(KvsType type)
{
    T_TRACEFUNC("");
    QMutexLocker locker(&mutex);
    TKvsDatabase db;

    if (maxConnectionsPerProcess() > 0) {
        QMap<QString, QDateTime> &map = pooledConnections[(int)type];
        QMap<QString, QDateTime>::iterator it = map.begin();

        while (it != map.end()) {
            db = TKvsDatabase::database(it.key());
            it = map.erase(it);
            if (db.isOpen()) {
                tSystemDebug("pop KVS database: %s", qPrintable(db.connectionName()));
                return db;
            } else {
                tSystemError("Pooled KVS database is not open: %s  [%s:%d]", qPrintable(db.connectionName()), __FILE__, __LINE__);
            }
        }

        for (int i = 0; i < maxConnectionsPerProcess(); ++i) {
            db = TKvsDatabase::database(QString().sprintf("%s_%d", qPrintable(driverName(type)), i));
            if (!db.isOpen()) {
                break;
            }
        }

        if (db.isOpen()) {
            throw RuntimeException("No pooled connection", __FILE__, __LINE__);
        }

        openDatabase(db, type, dbEnvironment);
        tSystemDebug("pop KVS database: %s", qPrintable(db.connectionName()));
    }
    return db;
}


bool TKvsDatabasePool::openDatabase(TKvsDatabase &database, KvsType type, const QString &env) const
{
    // Initiates database
    QSettings &settings = kvsSettings(type);
    settings.beginGroup(env);

    QString databaseName = settings.value("DatabaseName").toString().trimmed();
    if (databaseName.isEmpty()) {
        tError("KVS Database name empty string");
        settings.endGroup();
        return false;
    }
    tSystemDebug("KVS driver name: %s", qPrintable(database.driverName()));
    tSystemDebug("KVS DatabaseName: %s", qPrintable(databaseName));
    database.setDatabaseName(databaseName);

    QString hostName = settings.value("HostName").toString().trimmed();
    tSystemDebug("KVS HostName: %s", qPrintable(hostName));
    if (!hostName.isEmpty())
        database.setHostName(hostName);

    int port = settings.value("Port").toInt();
    tSystemDebug("KVS Port: %d", port);
    if (port > 0)
        database.setPort(port);

    QString userName = settings.value("UserName").toString().trimmed();
    tSystemDebug("KVS UserName: %s", qPrintable(userName));
    if (!userName.isEmpty())
        database.setUserName(userName);

    QString password = settings.value("Password").toString().trimmed();
    tSystemDebug("KVS Password: %s", qPrintable(password));
    if (!password.isEmpty())
        database.setPassword(password);

    QString connectOptions = settings.value("ConnectOptions").toString().trimmed();
    tSystemDebug("KVS ConnectOptions: %s", qPrintable(connectOptions));
    if (!connectOptions.isEmpty())
        database.setConnectOptions(connectOptions);

    settings.endGroup();

    if (!database.open()) {
        tError("KVS database open error");
        database = TKvsDatabase();
        return false;
    }

    tSystemDebug("KVS opened successfully (env:%s)", qPrintable(env));
    return true;
}


void TKvsDatabasePool::push(TKvsDatabase &database)
{
    T_TRACEFUNC("");
    QMutexLocker locker(&mutex);

    if (database.isValid()) {
        int type = kvsTypeHash()->value(database.driverName(), -1);
        if (type < 0) {
            throw RuntimeException("No such KVS type", __FILE__, __LINE__);
        }

        pooledConnections[type].insert(database.connectionName(), QDateTime::currentDateTime());
        tSystemDebug("push KVS database: %s", qPrintable(database.connectionName()));
    }
    database = TKvsDatabase();  // Sets an invalid object
}


void TKvsDatabasePool::timerEvent(QTimerEvent *event)
{
    T_TRACEFUNC("");

    if (event->timerId() == timer.timerId()) {
        // Closes extra-connection
        if (mutex.tryLock()) {
            for (int i = 0; i < pooledConnections.count(); ++i) {
                QMap<QString, QDateTime> &map = pooledConnections[i];
                QMap<QString, QDateTime>::iterator it = map.begin();

                while (it != map.end()) {
                    QDateTime dt = it.value();
                    if (dt.addSecs(30) < QDateTime::currentDateTime()) {
                        TKvsDatabase::database(it.key()).close();
                        tSystemDebug("Closed KVS database connection, name: %s", qPrintable(it.key()));
                        it = map.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            mutex.unlock();
        }
    } else {
        QObject::timerEvent(event);
    }
}


/*!
 * Initializes.
 * Call this in main thread.
 */
void TKvsDatabasePool::instantiate()
{
    if (!databasePool) {
        databasePool = new TKvsDatabasePool(Tf::app()->databaseEnvironment());
        databasePool->init();
        qAddPostRoutine(cleanup);
    }
}


TKvsDatabasePool *TKvsDatabasePool::instance()
{
    if (!databasePool) {
        tFatal("Call TKvsDatabasePool::initialize() function first");
    }
    return databasePool;
}


QString TKvsDatabasePool::driverName(TKvsDatabasePool::KvsType type)
{
    return kvsTypeHash()->key((int)type);
}


int TKvsDatabasePool::maxConnectionsPerProcess()
{
    return TSqlDatabasePool::maxConnectionsPerProcess();
}
