/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsessionmanager.h"
#include "tsessionstorefactory.h"
#include "tsystemglobal.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QHostInfo>
#include <QThread>
#include <TAppSettings>
#include <TAtomic>
#include <TSessionStore>


static QByteArray createHash()
{
    static TAtomic<quint64> seq(100000);
    QByteArray data;
    data.reserve(127);

    data.append(QByteArray::number(QDateTime::currentMSecsSinceEpoch()));
    data.append(QHostInfo::localHostName().toLatin1());
    data.append(QByteArray::number(++seq));
    data.append(QByteArray::number(QCoreApplication::applicationPid()));
    data.append(QByteArray::number((qulonglong)QThread::currentThread()));
    data.append(QByteArray::number((qulonglong)qApp));
    data.append(QByteArray::number((qulonglong)Tf::rand64_r()));
    return QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();
}


TSessionManager::TSessionManager()
{
}


TSessionManager::~TSessionManager()
{
}


TSession TSessionManager::findSession(const QByteArray &id)
{
    TSession session;

    if (!id.isEmpty()) {
        TSessionStore *store = TSessionStoreFactory::create(storeType());
        if (Q_LIKELY(store)) {
            session = store->find(id);
            TSessionStoreFactory::destroy(storeType(), store);
        } else {
            tSystemError("Session store not found: %s", qUtf8Printable(storeType()));
        }
    }
    return session;
}


bool TSessionManager::store(TSession &session)
{
    if (session.id().isEmpty()) {
        tSystemError("Internal Error  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    bool res = false;
    TSessionStore *store = TSessionStoreFactory::create(storeType());
    if (Q_LIKELY(store)) {
        res = store->store(session);
        TSessionStoreFactory::destroy(storeType(), store);
    } else {
        tSystemError("Session store not found: %s", qUtf8Printable(storeType()));
    }
    return res;
}


bool TSessionManager::remove(const QByteArray &id)
{
    if (!id.isEmpty()) {
        TSessionStore *store = TSessionStoreFactory::create(storeType());
        if (Q_LIKELY(store)) {
            bool ret = store->remove(id);
            TSessionStoreFactory::destroy(storeType(), store);
            return ret;
        } else {
            tSystemError("Session store not found: %s", qUtf8Printable(storeType()));
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
        id = createHash();  // Hash algorithm is important!
        if (findSession(id).isEmpty()) {
            break;
        }
    }

    if (i == 3) {
        throw RuntimeException("Unable to generate a unique session ID", __FILE__, __LINE__);
    }

    return id;
}


void TSessionManager::collectGarbage()
{
    static const int prob = Tf::appSettings()->value(Tf::SessionGcProbability).toInt();

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
                TSessionStoreFactory::destroy(storeType(), store);
            }
        }
    }
}


TSessionManager &TSessionManager::instance()
{
    static TSessionManager manager;
    return manager;
}
