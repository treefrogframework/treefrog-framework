/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QFile>
#include <QDir>
#include <QDataStream>
#include <TWebApplication>
#include "tsessionfilestore.h"

#define SESSION_DIR_NAME "session"

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

    bool res = false;
    QFile file(sessionDirPath() + session.id());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QDataStream ds(&file);
        ds << *static_cast<const QVariantMap *>(&session);
        res = (ds.status() == QDataStream::Ok);
    }
    return res;
}


TSession TSessionFileStore::find(const QByteArray &id, const QDateTime &modified)
{
    QFileInfo fi(sessionDirPath() + id);

    if (fi.exists() && fi.lastModified() >= modified) {
        QFile file(fi.filePath());

        if (file.open(QIODevice::ReadOnly)) {
            QDataStream ds(&file);
            TSession result(id);
            ds >> *static_cast<QVariantMap *>(&result);
            if (ds.status() == QDataStream::Ok)
                return result;
        }
    }
    return TSession();
}


bool TSessionFileStore::remove(const QDateTime &garbageExpiration)
{
    bool res = true;
    QDir dir(sessionDirPath());
    if (dir.exists()) {
        QList<QFileInfo> list = dir.entryInfoList(QDir::Files, QDir::Time | QDir::Reversed);
        for (QListIterator<QFileInfo> i(list); i.hasNext(); ) {
            const QFileInfo &fi = i.next();
            if (fi.lastModified() < garbageExpiration) {
                res &= dir.remove(fi.fileName());
            } else {
                break;
            }
        }
    }
    return res;
}


bool TSessionFileStore::remove(const QByteArray &id)
{
    return QFile::remove(sessionDirPath() + id);
}


QString TSessionFileStore::sessionDirPath()
{
    return Tf::app()->tmpPath() + QLatin1String(SESSION_DIR_NAME) + QDir::separator();
}
