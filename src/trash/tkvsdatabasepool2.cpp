/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QStringList>
#include <QDateTime>
#include <QHash>
#include <TWebApplication>
#include "tkvsdatabasepool2.h"
#include "tsqldatabasepool2.h"
#include "tatomicset.h"
#include "tsystemglobal.h"

/*!
  \class TKvsDatabasePool2
  \brief The TKvsDatabasePool2 class manages a collection of TKvsDatabase instances.
*/

#define CONN_NAME_FORMAT  "kvs%02d_%d"

static TKvsDatabasePool2 *databasePool = 0;


class KvsTypeHash : public QHash<QString, int>
{
public:
    KvsTypeHash() : QHash<QString, int>()
    {
        insert("MONGODB", TKvsDatabase::MongoDB);
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


TKvsDatabasePool2::TKvsDatabasePool2(const QString &environment)
    : QObject(), dbSet(0), maxConnects(0), dbEnvironment(environment)
{
    // Starts the timer to close extra-connection
    timer.start(10000, this);
}


TKvsDatabasePool2::~TKvsDatabasePool2()
{
    timer.stop();

    for (QHashIterator<QString, int> it(*kvsTypeHash()); it.hasNext(); ) {
        it.next();
        int type = it.value();

        if (!isKvsAvailable((TKvsDatabase::Type)type)) {
            tSystemDebug("KVS database not available. type:%d", (int)type);
            continue;
        }

        for (int i = 0; i < maxConnects; ++i) {
            QString dbName = QString().sprintf(CONN_NAME_FORMAT, type, i);

            DatabaseUse *du = (DatabaseUse *)dbSet[type].peekPop(i);
            if (du) {
                delete du;
            } else {
                tSystemWarn("Leak memory of DatabaseUse: %s", qPrintable(dbName));
            }
        }
    }

    TKvsDatabase::removeAllDatabases();

    if (dbSet)
        delete[] dbSet;
}


void TKvsDatabasePool2::init()
{
    if (dbSet)
        return;

    int typeCnt = kvsTypeHash()->count();
    if (typeCnt == 0)
        return;

    dbSet = new TAtomicSet[typeCnt];

    int typeidx = 0;
    for (QHashIterator<QString, int> it(*kvsTypeHash()); it.hasNext(); ) {
        const QString &drv = it.next().key();
        int type = it.value();

        if (!isKvsAvailable((TKvsDatabase::Type)type)) {
            tSystemDebug("KVS database not available. type:%d", (int)type);
            continue;
        } else {
            tSystemInfo("KVS database available. type:%d", (int)type);
        }

        dbSet[typeidx].setMaxCount(maxConnects);

        for (int i = 0; i < maxConnects; ++i) {
            // Adds databases previously
            QString dbName = QString().sprintf(CONN_NAME_FORMAT, type, i);
            TKvsDatabase db = TKvsDatabase::addDatabase(drv, dbName);
            if (!db.isValid()) {
                tWarn("KVS init parameter is invalid");
                break;
            }

            setDatabaseSettings(db, (TKvsDatabase::Type)type, dbEnvironment);

            DatabaseUse *du = new DatabaseUse;
            du->dbName = dbName;
            du->lastUsed = 0;
            if (dbSet[typeidx].push(du))
                tSystemDebug("Add KVS successfully. name:%s", qPrintable(db.connectionName()));
            else
                tSystemError("Failed to add KVS. name:%s", qPrintable(db.connectionName()));
        }
        typeidx++;
    }
}


bool TKvsDatabasePool2::isKvsAvailable(TKvsDatabase::Type type) const
{
    switch (type) {
    case TKvsDatabase::MongoDB:
        return Tf::app()->isMongoDbAvailable();
        break;
    default:
        throw RuntimeException("No such KVS type", __FILE__, __LINE__);
        break;
    }
}


QSettings &TKvsDatabasePool2::kvsSettings(TKvsDatabase::Type type) const
{
    switch (type) {
    case TKvsDatabase::MongoDB:
        if (Tf::app()->isMongoDbAvailable()) {
            return Tf::app()->mongoDbSettings();
        }
        break;
    default:
        throw RuntimeException("No such KVS type", __FILE__, __LINE__);
        break;
    }

    throw RuntimeException("Logic error", __FILE__, __LINE__);
}


TKvsDatabase TKvsDatabasePool2::database(TKvsDatabase::Type type)
{
    TKvsDatabase db;

    if (!isKvsAvailable(type))
        return db;

    DatabaseUse *du = (DatabaseUse *)dbSet[(int)type].pop();
    if (du) {
        db = TKvsDatabase::database(du->dbName);
        delete du;

        if (!db.isOpen()) {
            if (!db.open()) {
                tError("KVS open error. Invalid database settings, or maximum number of KVS connection exceeded.");
                tSystemError("KVS open error: %s", qPrintable(db.connectionName()));
                return db;
            }

            tSystemDebug("KVS opened successfully  env:%s connectname:%s dbname:%s", qPrintable(dbEnvironment),
                         qPrintable(db.connectionName()), qPrintable(db.databaseName()));
        }
        return db;
    }

    throw RuntimeException("No pooled connection", __FILE__, __LINE__);
}


bool TKvsDatabasePool2::setDatabaseSettings(TKvsDatabase &database, TKvsDatabase::Type type, const QString &env) const
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
    tSystemDebug("KVS db name:%s  driver name:%s", qPrintable(databaseName), qPrintable(database.driverName()));
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
    return true;
}


void TKvsDatabasePool2::pool(TKvsDatabase &database)
{
    if (database.isValid()) {
        int type = kvsTypeHash()->value(database.driverName(), -1);
        if (type < 0) {
            throw RuntimeException("No such KVS type", __FILE__, __LINE__);
        }

        DatabaseUse *du = new DatabaseUse;
        du->dbName = database.connectionName();
        du->lastUsed = QDateTime::currentDateTime().toTime_t();
        if (dbSet[type].push(du)) {
            tSystemDebug("Pooled KVS database: %s", qPrintable(database.connectionName()));
        } else {
            tSystemError("Failed to pool KVS database. %s", qPrintable(database.connectionName()));
            delete du;
        }
    }
    database = TKvsDatabase();  // Sets an invalid object
}


void TKvsDatabasePool2::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != timer.timerId()) {
        QObject::timerEvent(event);
        return;
    }

    int typeCnt = kvsTypeHash()->count();

    // Closes connection
    for (int j = 0; j < typeCnt; ++j) {
        for (int i = 0; i < dbSet[j].maxCount(); ++i) {
            DatabaseUse *du = (DatabaseUse *)dbSet[j].peekPop(i);
            if (du) {
                if (du->lastUsed < QDateTime::currentDateTime().toTime_t() - 30) {
                    TKvsDatabase db = TKvsDatabase::database(du->dbName);
                    if (db.isOpen()) {
                        db.close();
                        tSystemDebug("Closed KVS connection, name: %s", qPrintable(du->dbName));
                    }
                }
                dbSet[j].peekPush(du);
            }
        }
    }
}


/*!
 * Initializes.
 * Call this in main thread.
 */
void TKvsDatabasePool2::instantiate(int maxConnections)
{
    if (!databasePool) {
        databasePool = new TKvsDatabasePool2(Tf::app()->databaseEnvironment());
        databasePool->maxConnects = (maxConnections > 0) ? maxConnections : maxDbConnectionsPerProcess();
        databasePool->init();
        qAddPostRoutine(::cleanup);
    }
}


TKvsDatabasePool2 *TKvsDatabasePool2::instance()
{
    if (Q_UNLIKELY(!databasePool)) {
        tFatal("Call TKvsDatabasePool2::initialize() function first");
    }
    return databasePool;
}


QString TKvsDatabasePool2::driverName(TKvsDatabase::Type type)
{
    return kvsTypeHash()->key((int)type);
}


int TKvsDatabasePool2::maxDbConnectionsPerProcess()
{
    return TSqlDatabasePool2::maxDbConnectionsPerProcess();
}
