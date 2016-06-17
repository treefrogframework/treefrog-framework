/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QHostInfo>
#include <QCryptographicHash>
#include <QThread>
#include <QCoreApplication>
#include <TAppSettings>
#include <TSessionStore>
#include "tsystemglobal.h"
#include "tsessionmanager.h"
#include "tsessionstorefactory.h"


static QByteArray createHash()
{
    static quint32 seq = 0;
    QByteArray data;
    data.reserve(127);

#if QT_VERSION >= 0x040700
    data.append(QByteArray::number(QDateTime::currentMSecsSinceEpoch()));
#else
    QDateTime now = QDateTime::currentDateTime();
    data.append(QByteArray::number(now.toTime_t()));
    data.append(QByteArray::number(now.time().msec()));
#endif
    data.append(QHostInfo::localHostName());
    data.append(QByteArray::number(++seq));
    data.append(QByteArray::number(QCoreApplication::applicationPid()));
    data.append(QByteArray::number((qulonglong)QThread::currentThread()));
    data.append(QByteArray::number((qulonglong)qApp));
    data.append(QByteArray::number(Tf::rand32_r()));
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
    TSession session;

    if (!id.isEmpty()) {
        TSessionStore *store = TSessionStoreFactory::create(storeType());
        if (Q_LIKELY(store)) {
            store->setLifeTime(sessionLifeTime());
            session = store->find(id);
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
    if (Q_LIKELY(store)) {
        store->setLifeTime(sessionLifeTime());
        res = store->store(session);
        delete store;
    }
    return res;
}


bool TSessionManager::remove(const QByteArray &id)
{
    if (!id.isEmpty()) {
        TSessionStore *store = TSessionStoreFactory::create(storeType());
        if (Q_LIKELY(store)) {
            store->setLifeTime(sessionLifeTime());
            bool ret = store->remove(id);
            delete store;
            return ret;
        }
    }
    return false;
}


QString TSessionManager::storeType() const
{
    static QString type = Tf::appSettings()->value(Tf::SessionStoreType).toString().toLower();
    return type;
}


QByteArray TSessionManager::generateId()
{
    QByteArray id;
    int i;
    for (i = 0; i < 3; ++i) {
        id = createHash();   // Hash algorithm is important!
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
        prob = Tf::appSettings()->value(Tf::SessionGcProbability).toInt();
    }

    if (prob > 0) {
        int r = Tf::random(0, prob - 1);
        tSystemDebug("Session garbage collector : rand = %d", r);

        if (r == 0) {
            tSystemDebug("Session garbage collector started");

            TSessionStore *store = TSessionStoreFactory::create(storeType());
            if (store) {
                int gclifetime = Tf::appSettings()->value(Tf::SessionGcMaxLifeTime).toInt();
                QDateTime expire = QDateTime::currentDateTime().addSecs(-gclifetime);
                store->gc(expire);
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

    if (Q_UNLIKELY(lifetime < 0)) {
        lifetime = Tf::appSettings()->value(Tf::SessionLifeTime).toInt();
        if (lifetime == 0) {
            lifetime = Tf::appSettings()->value(Tf::SessionGcMaxLifeTime).toInt();
        }
    }
    return lifetime;
}
