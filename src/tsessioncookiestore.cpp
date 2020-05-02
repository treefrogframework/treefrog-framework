/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsessioncookiestore.h"
#include <QByteArray>
#include <QCryptographicHash>
#include <QDataStream>
#include <TAppSettings>
#include <TSystemGlobal>

/*!
  \class TSessionCookieStore
  \brief The TSessionCookieStore class stores HTTP sessions into a cookie.
*/


static const QByteArray &sessionSecret()
{
    static QByteArray secret = Tf::appSettings()->value(Tf::SessionSecret).toByteArray();
    return secret;
}


bool TSessionCookieStore::store(TSession &session)
{
    if (session.isEmpty()) {
        session.sessionId = "";
        return true;
    }

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

    ba = Tf::lz4Compress(ba);
    QByteArray digest = QCryptographicHash::hash(ba + sessionSecret(), QCryptographicHash::Sha1);
    session.sessionId = ba.toBase64() + "_" + digest.toBase64();
    return true;
}


TSession TSessionCookieStore::find(const QByteArray &id)
{
    TSession session(id);
    if (id.isEmpty()) {
        return session;
    }

    QByteArrayList balst = id.split('_');

    if (balst.count() == 2) {
        const auto &data = balst[0];
        const auto &dgstr = balst[1];

        if (!data.isEmpty() && !dgstr.isEmpty()) {
            QByteArray ba = QByteArray::fromBase64(data);
            QByteArray digest = QCryptographicHash::hash(ba + sessionSecret(), QCryptographicHash::Sha1);

            if (digest != QByteArray::fromBase64(dgstr)) {
                tSystemWarn("Recieved a tampered cookie or that of other web application.");
                //throw SecurityException("Tampered with cookie", __FILE__, __LINE__);
                return session;
            }

            ba = Tf::lz4Uncompress(ba);
            QDataStream ds(&ba, QIODevice::ReadOnly);
            ds >> *static_cast<QVariantMap *>(&session);

            if (ds.status() != QDataStream::Ok) {
                tSystemError("Failed to load a session from the cookie store.");
                session.reset();
            }
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
