/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMutexLocker>
#include <QFileInfo>
#include <QDir>
#include <TSqlDatabasePool>
#include <TWebApplication>
#include "tsystemglobal.h"

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
        QMap<QString, QDateTime> &map = pooledConnections[j];
        QMap<QString, QDateTime>::iterator it = map.begin();
        while (it != map.end()) {
            QSqlDatabase::database(it.key(), false).close();
            it = map.erase(it);
        }
        
        for (int i = 0; i < maxConnectionsPerProcess(); ++i) {
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
    : QObject(), dbEnvironment(environment)
{
    // Starts the timer to close extra-connection
    timer.start(10000, this);
}


void TSqlDatabasePool::init()
{
    // Adds databases previously
    //maxConnections = (Tf::app()->multiProcessingModule() == TWebApplication::Thread) ? Tf::app()->maxNumberOfServers() : 1;

    for (int j = 0; j < Tf::app()->sqlDatabaseSettingsCount(); ++j) {
        QString type = driverType(dbEnvironment, j);
        if (type.isEmpty()) {
            continue;
        }
        
        for (int i = 0; i < maxConnectionsPerProcess(); ++i) {
            QSqlDatabase db = QSqlDatabase::addDatabase(type, QString().sprintf("%02d_%d", j, i));
            if (!db.isValid()) {
                tWarn("Parameter 'DriverType' is invalid");
                break;
            }
            tSystemDebug("Add Database successfully. name:%s", qPrintable(db.connectionName())); 
        }

        pooledConnections.append(QMap<QString, QDateTime>());
    }
}


QSqlDatabase TSqlDatabasePool::pop(int databaseId)
{
    T_TRACEFUNC("");
    QMutexLocker locker(&mutex);

    QSqlDatabase db;
    if (databaseId < 0 || databaseId >= pooledConnections.count())
        return db;

    if (maxConnectionsPerProcess() > 0) {
        QMap<QString, QDateTime> &map = pooledConnections[databaseId];
        QMap<QString, QDateTime>::iterator it = map.begin();
        while (it != map.end()) {
            db = QSqlDatabase::database(it.key(), false);
            it = map.erase(it);
            if (db.isOpen()) {
                tSystemDebug("pop database: %s", qPrintable(db.connectionName()));
                return db;
            } else {
                tSystemError("Pooled database is not open: %s  [%s:%d]", qPrintable(db.connectionName()), __FILE__, __LINE__);
            }
        }
        
        for (int i = 0; i < maxConnectionsPerProcess(); ++i) {
            db = QSqlDatabase::database(QString().sprintf("%02d_%d", databaseId, i), false);
            if (!db.isOpen()) {
                break;
            }
        }
        
        if (db.isOpen()) {
            throw RuntimeException("No pooled connection", __FILE__, __LINE__);
        }
        
        openDatabase(db, dbEnvironment, databaseId);
        tSystemDebug("pop database: %s", qPrintable(db.connectionName()));
    }
    return db;
}


bool TSqlDatabasePool::openDatabase(QSqlDatabase &database, const QString &env, int databaseId)
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
    tSystemDebug("SQL driver name: %s", qPrintable(database.driverName()));
    tSystemDebug("DatabaseName: %s", qPrintable(databaseName));
    
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

    if (!database.open()) {
        tError("Database open error");
        database = QSqlDatabase();
        return false;
    }
    
    tSystemDebug("Database opened successfully (env:%s)", qPrintable(env));
    return true;
}


void TSqlDatabasePool::push(QSqlDatabase &database)
{
    T_TRACEFUNC("");
    QMutexLocker locker(&mutex);

    if (database.isValid()) {
        bool ok;
        int databaseId = database.connectionName().left(2).toInt(&ok);

        if (ok && databaseId >= 0 && databaseId < pooledConnections.count()) {
            pooledConnections[databaseId].insert(database.connectionName(), QDateTime::currentDateTime());
            tSystemDebug("push database: %s", qPrintable(database.connectionName()));
        } else {
            tSystemError("Invalid connection name: %s  [%s:%d]", qPrintable(database.connectionName()), __FILE__, __LINE__);
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
                QMap<QString, QDateTime> &map = pooledConnections[i];
                QMap<QString, QDateTime>::iterator it = map.begin();
                while (it != map.end()) {
                    QDateTime dt = it.value();
                    if (dt.addSecs(30) < QDateTime::currentDateTime()) {
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
        databasePool->init();
        qAddPostRoutine(cleanup);
    }
}


TSqlDatabasePool *TSqlDatabasePool::instance()
{
    if (!databasePool) {
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


int TSqlDatabasePool::maxConnectionsPerProcess()
{
    static int maxConnections = 0;

    if (!maxConnections) {
        maxConnections = (Tf::app()->multiProcessingModule() == TWebApplication::Thread) ? Tf::app()->maxNumberOfServers() : 1;
    }
    return maxConnections;
}
