/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QStringList>
#include <QMutexLocker>
#include <QDateTime>
#include <QHash>
#include <TWebApplication>
#include "tkvsdatabasepool.h"
#include "tsqldatabasepool.h"
#include "tsystemglobal.h"

/*!
  \class TKvsDatabasePool
  \brief The TKvsDatabasePool class manages a collection of TKvsDatabase instances.
*/

#define CONN_NAME_FORMAT  "kvs%02d_%d"

static TKvsDatabasePool *databasePool = 0;


class KvsTypeHash : public QHash<QString, int>
{
public:
    KvsTypeHash() : QHash<QString, int>()
    {
        insert("MONGODB", TKvsDatabase::MongoDB);
        insert("REDIS", TKvsDatabase::Redis);
    }
};
Q_GLOBAL_STATIC(KvsTypeHash, kvsTypeHash)


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
        QMap<QString, uint> &map = pooledConnections[j];
        QMap<QString, uint>::iterator it = map.begin();
        while (it != map.end()) {
            TKvsDatabase::database(it.key()).close();
            it = map.erase(it);
        }
    }

    TKvsDatabase::removeAllDatabases();
}


TKvsDatabasePool::TKvsDatabasePool(const QString &environment)
    : QObject(), maxConnects(0), dbEnvironment(environment)
{
    // Starts the timer to close extra-connection
    timer.start(10000, this);
}


void TKvsDatabasePool::init()
{
    // Adds databases previously

    for (QHashIterator<QString, int> it(*kvsTypeHash()); it.hasNext(); ) {
        const QString &drv = it.next().key();
        int type = it.value();
        pooledConnections.append(QMap<QString, uint>());

        if (!isKvsAvailable((TKvsDatabase::Type)type)) {
            tSystemDebug("KVS database not available. type:%d", (int)type);
            continue;
        } else {
            tSystemInfo("KVS database available. type:%d", (int)type);
        }

        for (int i = 0; i < maxConnects; ++i) {
            TKvsDatabase db = TKvsDatabase::addDatabase(drv, QString().sprintf(CONN_NAME_FORMAT, type, i));
            if (!db.isValid()) {
                tWarn("KVS init parameter is invalid");
                break;
            }

            setDatabaseSettings(db, (TKvsDatabase::Type)type, dbEnvironment);
            tSystemDebug("Add KVS successfully. name:%s", qPrintable(db.connectionName()));
        }
    }
}


bool TKvsDatabasePool::isKvsAvailable(TKvsDatabase::Type type) const
{
    switch (type) {
    case TKvsDatabase::MongoDB:
        return Tf::app()->isMongoDbAvailable();
        break;

    case TKvsDatabase::Redis:
        return Tf::app()->isRedisAvailable();
        break;

    default:
        throw RuntimeException("No such KVS type", __FILE__, __LINE__);
        break;
    }
}


QSettings &TKvsDatabasePool::kvsSettings(TKvsDatabase::Type type) const
{
    switch (type) {
    case TKvsDatabase::MongoDB:
        if (Tf::app()->isMongoDbAvailable()) {
            return Tf::app()->mongoDbSettings();
        }
        break;

    case TKvsDatabase::Redis:
        if (Tf::app()->isRedisAvailable()) {
            return Tf::app()->redisSettings();
        }
        break;


    default:
        throw RuntimeException("No such KVS type", __FILE__, __LINE__);
        break;
    }

    throw RuntimeException("Logic error", __FILE__, __LINE__);
}


TKvsDatabase TKvsDatabasePool::database(TKvsDatabase::Type type)
{
    T_TRACEFUNC("");

    QMutexLocker locker(&mutex);
    TKvsDatabase db;

    if (!isKvsAvailable(type)) {
        switch (type) {
        case TKvsDatabase::MongoDB:
            tSystemError("MongoDB not available. Check the settings file.");
            break;

        case TKvsDatabase::Redis:
            tSystemError("Redis not available. Check the settings file.");
            break;

        default:
            throw RuntimeException("No such KVS type", __FILE__, __LINE__);
            break;
        }
        return db;
    }

    QMap<QString, uint> &map = pooledConnections[(int)type];
    QMap<QString, uint>::iterator it = map.begin();
    while (it != map.end()) {
        db = TKvsDatabase::database(it.key());
        it = map.erase(it);
        if (Q_LIKELY(db.isOpen())) {
            tSystemDebug("Gets KVS database: %s", qPrintable(db.connectionName()));
            return db;
        } else {
            tSystemError("Pooled KVS database is not open: %s  [%s:%d]", qPrintable(db.connectionName()), __FILE__, __LINE__);
        }
    }

    for (int i = 0; i < maxConnects; ++i) {
        db = TKvsDatabase::database(QString().sprintf(CONN_NAME_FORMAT, (int)type, i));
        if (!db.isOpen()) {
            if (Q_UNLIKELY(!db.open())) {
                tError("KVS database open error");
                tSystemError("KVS Database open error: %s", qPrintable(db.connectionName()));
                return TKvsDatabase();
            }
            tSystemDebug("KVS opened successfully  env:%s connectname:%s dbname:%s", qPrintable(dbEnvironment), qPrintable(db.connectionName()), qPrintable(db.databaseName()));
            return db;
        }
    }

    throw RuntimeException("No pooled connection", __FILE__, __LINE__);
}


bool TKvsDatabasePool::setDatabaseSettings(TKvsDatabase &database, TKvsDatabase::Type type, const QString &env) const
{
    // Initiates database
    QSettings &settings = kvsSettings(type);
    settings.beginGroup(env);

    QString databaseName = settings.value("DatabaseName").toString().trimmed();
    if (databaseName.isEmpty()) {
        if (type != TKvsDatabase::Redis) {
            tWarn("KVS Database name empty string");
            settings.endGroup();
            return false;
        }
    } else {
        tSystemDebug("KVS db name:%s  driver name:%s", qPrintable(databaseName), qPrintable(database.driverName()));
        database.setDatabaseName(databaseName);
    }

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
    return true;
}


void TKvsDatabasePool::pool(TKvsDatabase &database)
{
    T_TRACEFUNC("");
    QMutexLocker locker(&mutex);

    if (Q_LIKELY(database.isValid())) {
        int type = kvsTypeHash()->value(database.driverName(), -1);
        if (Q_UNLIKELY(type < 0)) {
            throw RuntimeException("No such KVS type", __FILE__, __LINE__);
        }

        pooledConnections[type].insert(database.connectionName(), QDateTime::currentDateTime().toTime_t());
        tSystemDebug("Pooled KVS database: %s", qPrintable(database.connectionName()));
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
                QMap<QString, uint> &map = pooledConnections[i];
                QMap<QString, uint>::iterator it = map.begin();

                while (it != map.end()) {
                    uint tm = it.value();
                    if (tm < QDateTime::currentDateTime().toTime_t() - 30) {  // 30sec
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
        databasePool->maxConnects = maxDbConnectionsPerProcess();
        databasePool->init();
        qAddPostRoutine(::cleanup);
    }
}


TKvsDatabasePool *TKvsDatabasePool::instance()
{
    if (Q_UNLIKELY(!databasePool)) {
        tFatal("Call TKvsDatabasePool::initialize() function first");
    }
    return databasePool;
}


QString TKvsDatabasePool::driverName(TKvsDatabase::Type type)
{
    return kvsTypeHash()->key((int)type);
}


int TKvsDatabasePool::maxDbConnectionsPerProcess()
{
    return TSqlDatabasePool::maxDbConnectionsPerProcess();
}
