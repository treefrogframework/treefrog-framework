/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QHostInfo>
#include <QCryptographicHash>
#include <QThread>
#include <TWebApplication>
#include <TSessionStore>
#include "tsystemglobal.h"
#include "tsessionmanager.h"
#include "tsessionstorefactory.h"

#define STORE_TYPE          "Session.StoreType"
#define GC_PROBABILITY      "Session.GcProbability"
#define GC_MAX_LIFE_TIME    "Session.GcMaxLifeTime"
#define SESSION_LIFETIME    "Session.LifeTime"


static QByteArray randomString()
{
    QByteArray data;
    data.reserve(128);

    QDateTime now = QDateTime::currentDateTime();
    data.append(QByteArray::number(now.toTime_t()));
    data.append(QByteArray::number(now.time().msec()));
    data.append(QHostInfo::localHostName());
    data.append(QByteArray::number(QCoreApplication::applicationPid()));
    data.append(QByteArray::number((qulonglong)QThread::currentThread()));
    data.append(QByteArray::number((qulonglong)qApp));
    data.append(QByteArray::number(Tf::randXor128()));
    return QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();
}


TSessionManager::TSessionManager()
{ }


TSessionManager::~TSessionManager()
{ }


TSession TSessionManager::findSession(const QByteArray &id)
{
    T_TRACEFUNC("");

    QDateTime now = QDateTime::currentDateTime();
    QDateTime validCreated = (sessionLifeTime() > 0) ? now.addSecs(-sessionLifeTime()) : now.addYears(-20);
    
    TSession session;
    if (!id.isEmpty()) {
        TSessionStore *store = TSessionStoreFactory::create(storeType());
        if (store) {
            session = store->find(id, validCreated);
            delete store;
        }
    }
    return session;
}


bool TSessionManager::store(TSession &session)
{
    T_TRACEFUNC("");

    if (session.id().isEmpty()) {
        tSystemError("Internal Error  [%s:%d]", __FILE__, __LINE__); 
        return false;
    }
    
    bool res = false;
    TSessionStore *store = TSessionStoreFactory::create(storeType());
    if (store) {
        res = store->store(session);
        delete store;
    }
    return res;
}


bool TSessionManager::remove(const QByteArray &id)
{
    if (!id.isEmpty()) {
        TSessionStore *store = TSessionStoreFactory::create(storeType());
        if (store) {
            bool ret = store->remove(id);
            delete store;
            return ret;
        }
    }
    return false;
}


QString TSessionManager::storeType() const
{
    return Tf::app()->appSettings().value(STORE_TYPE).toString().toLower();
}


QByteArray TSessionManager::generateId()
{
    QByteArray id;
    int i;
    for (i = 0; i < 3; ++i) {
        id = randomString();
        if (findSession(id).isEmpty())
            break;
    }

    if (i == 3)
        throw RuntimeException("Unable to generate a unique session ID", __FILE__, __LINE__);
    
    return id;
}


void TSessionManager::collectGarbage()
{
    static int prob = -1;

    if (prob == -1) {
        prob = Tf::app()->appSettings().value(GC_PROBABILITY).toInt();
    }

    if (prob > 0) {
        int r = Tf::random(prob - 1);
        tSystemDebug("Session garbage collector : rand = %d", r);

        if (r == 0) {
            tSystemDebug("Session garbage collector started");
            
            TSessionStore *store = TSessionStoreFactory::create(Tf::app()->appSettings().value(STORE_TYPE).toString());
            if (store) {
                int lifetime = Tf::app()->appSettings().value(GC_MAX_LIFE_TIME).toInt();
                store->remove(QDateTime::currentDateTime().addSecs(-lifetime));
                delete store;
            }
        }
    }
}


TSessionManager &TSessionManager::instance()
{
    static TSessionManager manager;
    return manager;
}


int TSessionManager::sessionLifeTime()
{
    static int lifetime = -1;

    if (lifetime < 0) {
        lifetime = Tf::app()->appSettings().value(SESSION_LIFETIME).toInt();
    }
    return lifetime;
}
