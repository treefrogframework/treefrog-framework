/* Copyright (c) 2010, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tactionroutine.h"
//#include "TWebApplication"
//#include "TSqlDatabasePool"

// static TActionRoutineManager *manager = 0;  // For parent process
// TActionRoutine *TActionRoutine::currentActionRoutine = 0;  // For child process


// TActionRoutineManager::TActionRoutineManager() : QObject()
// {
//     timer.start(1000, this);
// }


// TActionRoutineManager::~TActionRoutineManager()
// { }


// TActionRoutineManager *TActionRoutineManager::instance()
// {
//     return manager;
// }


// static void cleanupManager()
// {
//     if (manager) {
//         delete manager;
//         manager = 0;
//     }
// }


TActionRoutine::TActionRoutine() :
    TActionContext()
{
    // if (!manager) {
    //     manager = new TActionRoutineManager();
    //     qAddPostRoutine(cleanupManager);
    // }
    // setParent(manager);
}


TActionRoutine::~TActionRoutine()
{
//    closeDatabase();
}


THttpResponse TActionRoutine::start(THttpRequest &request)
{
    execute(request);

    return THttpResponse();
}

// bool TActionRoutine::openDatabase()
// {
//     QString env = tWebApp->databaseEnvironment();
//     QString type = TSqlDatabasePool::driverType(env);
//     if (type.isEmpty()) {
//         return false;
//     }

//     sqlDatabase = QSqlDatabase::addDatabase(type);
//     TSqlDatabasePool::openDatabase(sqlDatabase, env);
//     return sqlDatabase.isValid();
// }


// void TActionRoutine::closeDatabase()
// {
//     if (sqlDatabase.isValid()) {
//         sqlDatabase.rollback();
//         sqlDatabase.close();
//         sqlDatabase = QSqlDatabase();  // Sets an invalid object
//     }
// }


// void TActionRoutine::emitError(int socketError)
// {
//     emit error(socketError);
// }


// TActionRoutine *TActionRoutine::currentProcess()
// {
//     return currentActionRoutine;
// }


// bool TActionRoutine::isChildProcess()
// {
//     return (bool)currentActionRoutine;
// }


// void TActionRoutine::terminate(int status)
// {
//     tSystemDebug("Child process({}) teminated. status:{}", childPid, status);
//     emit finished();
// }


// void TActionRoutine::kill(int sig)
// {
//     tSystemDebug("Child process({}) killed. signal:{}", childPid, sig);
//     emit finished();
// }
