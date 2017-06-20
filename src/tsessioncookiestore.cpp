/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QByteArray>
#include <QDataStream>
#include <QCryptographicHash>
#include <TAppSettings>
#include <TSystemGlobal>
#include "tsessioncookiestore.h"

/*!
  \class TSessionCookieStore
  \brief The TSessionCookieStore class stores HTTP sessions into a cookie.
*/

bool TSessionCookieStore::store(TSession &session)
{
    if (session.isEmpty())
        return true;

#ifndef TF_NO_DEBUG
    {
        QByteArray badummy;
        QDataStream dsdmy(&badummy, QIODevice::ReadWrite);
        dsdmy << *static_cast<const QVariantMap *>(&session);

        TSession dummy;
        dsdmy.device()->seek(0);
        dsdmy >> *static_cast<QVariantMap *>(&dummy);
        if (dsdmy.status() != QDataStream::Ok) {
            tSystemError("Failed to store a session into the cookie store. Must set objects that can be serialized.");
        }
    }
#endif

    QByteArray ba;
    QDataStream ds(&ba, QIODevice::WriteOnly);
    ds << *static_cast<const QVariantMap *>(&session);
    if (ds.status() != QDataStream::Ok) {
        tSystemError("Failed to store session. Must set objects that can be serialized.");
        return false;
    }

    QByteArray digest = QCryptographicHash::hash(ba + Tf::appSettings()->value(Tf::SessionSecret).toByteArray(),
                                                 QCryptographicHash::Sha1);
    session.sessionId = ba.toHex() + "_" + digest.toHex();
    return true;
}


TSession TSessionCookieStore::find(const QByteArray &id)
{
    TSession session(id);
    if (id.isEmpty()) {
        return session;
    }

    QList<QByteArray> balst = id.split('_');
    if (balst.count() == 2 && !balst.value(0).isEmpty() && !balst.value(1).isEmpty()) {
        QByteArray ba = QByteArray::fromHex(balst.value(0));
        QByteArray digest = QCryptographicHash::hash(ba + Tf::appSettings()->value(Tf::SessionSecret).toByteArray(),
                                                     QCryptographicHash::Sha1);

        if (digest != QByteArray::fromHex(balst.value(1))) {
            tSystemWarn("Recieved a tampered cookie or that of other web application.");
            //throw SecurityException("Tampered with cookie", __FILE__, __LINE__);
            return session;
        }

        QDataStream ds(&ba, QIODevice::ReadOnly);
        ds >> *static_cast<QVariantMap *>(&session);

        if (ds.status() != QDataStream::Ok) {
            tSystemError("Failed to load a session from the cookie store.");
            session.reset();
        }
    }
    return session;
}


bool TSessionCookieStore::remove(const QByteArray &)
{
    return true;
}


int TSessionCookieStore::gc(const QDateTime &)
{
    return 0;
}
