/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMutexLocker>
#include <QFileInfo>
#include <QDir>
#include <TWebApplication>
#include <TAppSettings>
#include "tsqldatabasepool.h"
#include "tsystemglobal.h"

#define CONN_NAME_FORMAT  "rdb%02d_%d"

static TSqlDatabasePool *databasePool = 0;


static void cleanup()
{
    if (databasePool) {
        delete databasePool;
        databasePool = 0;
    }
}


TSqlDatabasePool::~TSqlDatabasePool()
{
    timer.stop();

    QMutexLocker locker(&mutex);
    for (int j = 0; j < pooledConnections.count(); ++j) {
        QMap<QString, uint> &map = pooledConnections[j];
        QMap<QString, uint>::iterator it = map.begin();
        while (it != map.end()) {
            QSqlDatabase::database(it.key(), false).close();
            it = map.erase(it);
        }

        for (int i = 0; i < maxConnects; ++i) {
            QString name = QString::number(j) + '_' + QString::number(i);
            if (QSqlDatabase::contains(name)) {
                QSqlDatabase::removeDatabase(name);
            } else {
                break;
            }
        }
    }
}


TSqlDatabasePool::TSqlDatabasePool(const QString &environment)
    : QObject(), maxConnects(0), dbEnvironment(environment)
{
    // Starts the timer to close extra-connection
    timer.start(10000, this);
}


void TSqlDatabasePool::init()
{
    if (!Tf::app()->isSqlDatabaseAvailable()) {
        tSystemWarn("SQL database not available");
        return;
    } else {
        tSystemDebug("SQL database available");
    }

    // Adds databases previously
    for (int j = 0; j < Tf::app()->sqlDatabaseSettingsCount(); ++j) {
        QString type = driverType(dbEnvironment, j);
        if (type.isEmpty()) {
            continue;
        }

        for (int i = 0; i < maxConnects; ++i) {
            QSqlDatabase db = QSqlDatabase::addDatabase(type, QString().sprintf(CONN_NAME_FORMAT, j, i));
            if (!db.isValid()) {
                tWarn("Parameter 'DriverType' is invalid");
                break;
            }

            setDatabaseSettings(db, dbEnvironment, j);
            tSystemDebug("Add Database successfully. name:%s", qPrintable(db.connectionName()));
        }

        pooledConnections.append(QMap<QString, uint>());
    }
}


QSqlDatabase TSqlDatabasePool::database(int databaseId)
{
    T_TRACEFUNC("");
    QMutexLocker locker(&mutex);
    QSqlDatabase db;

    if (Q_UNLIKELY(!Tf::app()->isSqlDatabaseAvailable())) {
        return db;
    }

    if (databaseId >= 0 && databaseId < pooledConnections.count()) {
        QMap<QString, uint> &map = pooledConnections[databaseId];
        QMap<QString, uint>::iterator it = map.begin();
        while (it != map.end()) {
            db = QSqlDatabase::database(it.key(), false);
            it = map.erase(it);
            if (Q_LIKELY(db.isOpen())) {
                tSystemDebug("Gets database: %s", qPrintable(db.connectionName()));
                return db;
            } else {
                tSystemError("Pooled database is not open: %s  [%s:%d]", qPrintable(db.connectionName()), __FILE__, __LINE__);
            }
        }

        for (int i = 0; i < maxConnects; ++i) {
            db = QSqlDatabase::database(QString().sprintf(CONN_NAME_FORMAT, databaseId, i), false);
            if (!db.isOpen()) {
                if (Q_UNLIKELY(!db.open())) {
                    tError("Database open error. Invalid database settings, or maximum number of SQL connection exceeded.");
                    tSystemError("SQL database open error: %s", qPrintable(db.connectionName()));
                    return QSqlDatabase();
                }

                tSystemDebug("SQL database opened successfully (env:%s)", qPrintable(dbEnvironment));
                tSystemDebug("Gets database: %s", qPrintable(db.connectionName()));
                return db;
            }
        }
    }

    throw RuntimeException("No pooled connection", __FILE__, __LINE__);
}


bool TSqlDatabasePool::setDatabaseSettings(QSqlDatabase &database, const QString &env, int databaseId)
{
    // Initiates database
    QSettings &settings = Tf::app()->sqlDatabaseSettings(databaseId);
    settings.beginGroup(env);

    QString databaseName = settings.value("DatabaseName").toString().trimmed();
    if (databaseName.isEmpty()) {
        tError("Database name empty string");
        settings.endGroup();
        return false;
    }
    tSystemDebug("SQL driver name:%s  dbname:%s", qPrintable(database.driverName()), qPrintable(databaseName));

    if (database.driverName().toUpper().startsWith("QSQLITE")) {
        QFileInfo fi(databaseName);
        if (fi.isRelative()) {
            // For SQLite
            databaseName = Tf::app()->webRootPath() + databaseName;
        }
    }
    database.setDatabaseName(databaseName);

    QString hostName = settings.value("HostName").toString().trimmed();
    tSystemDebug("Database HostName: %s", qPrintable(hostName));
    if (!hostName.isEmpty())
        database.setHostName(hostName);

    int port = settings.value("Port").toInt();
    tSystemDebug("Database Port: %d", port);
    if (port > 0)
        database.setPort(port);

    QString userName = settings.value("UserName").toString().trimmed();
    tSystemDebug("Database UserName: %s", qPrintable(userName));
    if (!userName.isEmpty())
        database.setUserName(userName);

    QString password = settings.value("Password").toString().trimmed();
    tSystemDebug("Database Password: %s", qPrintable(password));
    if (!password.isEmpty())
        database.setPassword(password);

    QString connectOptions = settings.value("ConnectOptions").toString().trimmed();
    tSystemDebug("Database ConnectOptions: %s", qPrintable(connectOptions));
    if (!connectOptions.isEmpty())
        database.setConnectOptions(connectOptions);

    settings.endGroup();
    return true;
}


void TSqlDatabasePool::pool(QSqlDatabase &database)
{
    T_TRACEFUNC("");
    QMutexLocker locker(&mutex);

    if (database.isValid()) {
        int databaseId = getDatabaseId(database);

        if (databaseId >= 0 && databaseId < pooledConnections.count()) {
            pooledConnections[databaseId].insert(database.connectionName(), QDateTime::currentDateTime().toTime_t());
            tSystemDebug("Pooled database: %s", qPrintable(database.connectionName()));
        } else {
            tSystemError("Pooled invalid database  [%s:%d]", __FILE__, __LINE__);
        }
    }
    database = QSqlDatabase();  // Sets an invalid object
}


void TSqlDatabasePool::timerEvent(QTimerEvent *event)
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
                    if (tm < QDateTime::currentDateTime().toTime_t() - 30) { // 30sec
                        QSqlDatabase::database(it.key(), false).close();
                        tSystemDebug("Closed database connection, name: %s", qPrintable(it.key()));
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
void TSqlDatabasePool::instantiate()
{
    if (!databasePool) {
        databasePool = new TSqlDatabasePool(Tf::app()->databaseEnvironment());
        databasePool->maxConnects = maxDbConnectionsPerProcess();
        databasePool->init();
        qAddPostRoutine(::cleanup);
    }
}


TSqlDatabasePool *TSqlDatabasePool::instance()
{
    if (Q_UNLIKELY(!databasePool)) {
        tFatal("Call TSqlDatabasePool::initialize() function first");
    }
    return databasePool;
}


QString TSqlDatabasePool::driverType(const QString &env, int databaseId)
{
    QSettings &settings = Tf::app()->sqlDatabaseSettings(databaseId);
    settings.beginGroup(env);
    QString type = settings.value("DriverType").toString().trimmed();
    settings.endGroup();

    if (type.isEmpty()) {
        tDebug("Parameter 'DriverType' is empty");
    }
    return type;
}


int TSqlDatabasePool::maxDbConnectionsPerProcess()
{
    int maxConnections = 0;
    QString mpm = Tf::appSettings()->value(Tf::MultiProcessingModule).toString().toLower();

    switch (Tf::app()->multiProcessingModule()) {
    case TWebApplication::Thread:
        maxConnections = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxThreadsPerAppServer").toInt();
        if (maxConnections <= 0) {
            maxConnections = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxServers", "128").toInt();
        }
        break;

    case TWebApplication::Hybrid:
        maxConnections = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxWorkersPerAppServer").toInt();
        if (maxConnections <= 0) {
            maxConnections = Tf::appSettings()->readValue(QLatin1String("MPM.") + mpm + ".MaxWorkersPerServer", "128").toInt();
        }
        break;

    default:
        break;
    }

    return maxConnections;
}


int TSqlDatabasePool::getDatabaseId(const QSqlDatabase &database)
{
    bool ok;
    int id = database.connectionName().mid(3,2).toInt(&ok);

    if (Q_LIKELY(ok && id >= 0)) {
        return id;
    }
    return -1;
}
