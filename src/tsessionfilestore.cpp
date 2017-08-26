/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QFile>
#include <QDir>
#include <QDataStream>
#include <QReadWriteLock>
#include <TWebApplication>
#include "tsessionfilestore.h"
#include "tsystemglobal.h"
#include "tfcore.h"

#define SESSION_DIR_NAME "session"

static QReadWriteLock rwLock(QReadWriteLock::Recursive);  // Global read-write lock

/*!
  \class TSessionFileStore
  \brief The TSessionFileStore class stores HTTP sessions to files.
*/

bool TSessionFileStore::store(TSession &session)
{
    QDir dir(sessionDirPath());
    if (!dir.exists()) {
        dir.mkpath(".");
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

    bool res = false;
    QWriteLocker locker(&rwLock);  // lock for threads
    QFile file(sessionDirPath() + session.id());

    if (file.open(QIODevice::ReadWrite)) {
        auto reslock = tf_lockfile(file.handle(), true, true);  // blocking flock for processes
        int err = errno;
        if (reslock < 0) {
            tSystemWarn("flock error  errno:%d", err);
        }

        file.resize(0); // truncate
        QDataStream ds(&file);
        ds << *static_cast<const QVariantMap *>(&session);
        file.close();

        res = (ds.status() == QDataStream::Ok);
        if (!res) {
            tSystemError("Failed to store session. Must set objects that can be serialized.");
        }
    }
    return res;
}


TSession TSessionFileStore::find(const QByteArray &id)
{
    QFileInfo fi(sessionDirPath() + id);
    QDateTime modified = QDateTime::currentDateTime().addSecs(-lifeTimeSecs());

    if (fi.exists() && fi.lastModified() >= modified) {
        QReadLocker locker(&rwLock);  // lock for threads
        QFile file(fi.filePath());

        if (file.open(QIODevice::ReadOnly)) {
            auto reslock = tf_lockfile(file.handle(), false, true);  // blocking flock for processes
            int err = errno;
            if (reslock < 0) {
                tSystemWarn("flock error  errno:%d", err);
            }

            QDataStream ds(&file);
            TSession result(id);
            ds >> *static_cast<QVariantMap *>(&result);
            file.close();

            if (ds.status() == QDataStream::Ok) {
                return result;
            } else {
                tSystemError("Failed to load a session from the file store.");
            }
        }
    }
    return TSession();
}


bool TSessionFileStore::remove(const QByteArray &id)
{
    return QFile::remove(sessionDirPath() + id);
}


int TSessionFileStore::gc(const QDateTime &expire)
{
    int res = 0;
    QDir dir(sessionDirPath());
    if (dir.exists()) {
        const QList<QFileInfo> lst = dir.entryInfoList(QDir::Files, QDir::Time | QDir::Reversed);
        for (auto &fi : lst) {
            if (fi.lastModified() < expire) {
                if (dir.remove(fi.fileName())) {
                    res++;
                }
            } else {
                break;
            }
        }
    }
    return res;
}


QString TSessionFileStore::sessionDirPath()
{
    return Tf::app()->tmpPath() + QLatin1String(SESSION_DIR_NAME) + QDir::separator();
}
