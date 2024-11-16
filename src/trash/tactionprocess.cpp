/* Copyright (c) 2010, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TActionProcess>
#include <TWebApplication>
#include <TSqlDatabasePool>

static TActionProcessManager *manager = 0;  // For parent process
TActionProcess *TActionProcess::currentActionProcess = 0;  // For child process


TActionProcessManager::TActionProcessManager() : QObject()
{
    timer.start(1000, this);
}


TActionProcessManager::~TActionProcessManager()
{ }


TActionProcessManager *TActionProcessManager::instance()
{
    return manager;
}


static void cleanupManager()
{
    if (manager) {
        delete manager;
        manager = 0;
    }
}


TActionProcess::TActionProcess(int socket)
    : QObject(), TActionContext(socket), childPid(-1)
{
    if (!manager) {
        manager = new TActionProcessManager();
        qAddPostRoutine(cleanupManager);
    }
    setParent(manager);
}


TActionProcess::~TActionProcess()
{
    closeDatabase();
}


bool TActionProcess::openDatabase()
{
    QString env = tWebApp->databaseEnvironment();
    QString type = TSqlDatabasePool::driverType(env);
    if (type.isEmpty()) {
        return false;
    }

    sqlDatabase = QSqlDatabase::addDatabase(type);
    TSqlDatabasePool::openDatabase(sqlDatabase, env);
    return sqlDatabase.isValid();
}


void TActionProcess::closeDatabase()
{
    if (sqlDatabase.isValid()) {
        sqlDatabase.rollback();
        sqlDatabase.close();
        sqlDatabase = QSqlDatabase();  // Sets an invalid object
    }
}


void TActionProcess::emitError(int socketError)
{
    emit error(socketError);
}


TActionProcess *TActionProcess::currentProcess()
{
    return currentActionProcess;
}


bool TActionProcess::isChildProcess()
{
    return (bool)currentActionProcess;
}


void TActionProcess::terminate(int status)
{
    tSystemDebug("Child process({}) teminated. status:{}", childPid, status); 
    emit finished();
}


void TActionProcess::kill(int sig)
{
    tSystemDebug("Child process({}) killed. signal:{}", childPid, sig); 
    emit finished();
}
