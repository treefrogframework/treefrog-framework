/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsessionstorefactory.h"
#include "tsessioncookiestore.h"
#include "tsessionfilestore.h"
#include "tsessionmongostore.h"
#include "tsessionredisstore.h"
#include "tsessionsqlobjectstore.h"
#include "tsystemglobal.h"
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QPluginLoader>
#include <QRegularExpression>
#include <TSessionStorePlugin>
#include <TWebApplication>

namespace {
QString COOKIE_SESSION_KEY;
QString SQLOBJECT_SESSION_KEY;
QString FILE_SESSION_KEY;
QString REDIS_SESSION_KEY;
QString MONGODB_SESSION_KEY;


void loadKeys()
{
    static bool done = []() {
        // Constants
        COOKIE_SESSION_KEY = TSessionCookieStore().key().toLower();
        SQLOBJECT_SESSION_KEY = TSessionSqlObjectStore().key().toLower();
        FILE_SESSION_KEY = TSessionFileStore().key().toLower();
        REDIS_SESSION_KEY = TSessionRedisStore().key().toLower();
        MONGODB_SESSION_KEY = TSessionMongoStore().key().toLower();
        return true;
    }();
    Q_UNUSED(done);
}
}

/*!
  \class TSessionStoreFactory
  \brief The TSessionStoreFactory class creates TSessionStore objects.
*/

/*!
  Loads session store plugins in the plugin directory and returns a pointer to QMap instance.
*/
static QMap<QString, TSessionStoreInterface *> *sessionStoreIfMap()
{
    static QMap<QString, TSessionStoreInterface *> *sessIfMap = []() {
        auto ifMap = new QMap<QString, TSessionStoreInterface *>();

        QDir dir(Tf::app()->pluginPath());
        const QStringList list = dir.entryList(QDir::Files);
        for (auto &file : list) {
            QPluginLoader loader(dir.absoluteFilePath(file));

            tSystemDebug("plugin library for session store: %s", qUtf8Printable(loader.fileName()));
            if (!loader.load()) {
                tSystemError("plugin load error: %s", qUtf8Printable(loader.errorString()));
                continue;
            }

            TSessionStoreInterface *iface = dynamic_cast<TSessionStoreInterface *>(loader.instance());
            if (iface) {
                const QVariantList array = loader.metaData().value("MetaData").toObject().value("Keys").toArray().toVariantList();
                for (auto &k : array) {
                    QString key = k.toString().toLower();
                    tSystemInfo("Loaded session store plugin: %s", qUtf8Printable(key));
                    ifMap->insert(key, iface);
                }
            }
        }
        return ifMap;
    }();
    return sessIfMap;
}

/*!
  Returns the list of valid keys, i.e.\ the available session stores.
*/
QStringList TSessionStoreFactory::keys()
{
    QStringList ret;

    loadKeys();
    ret << COOKIE_SESSION_KEY
        << SQLOBJECT_SESSION_KEY
        << FILE_SESSION_KEY
        << REDIS_SESSION_KEY
        << MONGODB_SESSION_KEY
        << sessionStoreIfMap()->keys();

    ret = ret.filter(QRegularExpression("\\S"));
    return ret;
}

/*!
  Creates and returns a TSessionStore object that matches the given key,
  or returns 0 if no matching session store is found.
*/
TSessionStore *TSessionStoreFactory::create(const QString &key)
{
    TSessionStore *ret = nullptr;
    loadKeys();

    if (key.isEmpty()) {
        return ret;
    }

    QString k = key.toLower();
    if (k == COOKIE_SESSION_KEY) {
        static TSessionCookieStore cookieStore;
        ret = &cookieStore;
    } else if (k == SQLOBJECT_SESSION_KEY) {
        static TSessionSqlObjectStore sqlObjectStore;
        ret = &sqlObjectStore;
    } else if (k == FILE_SESSION_KEY) {
        static TSessionFileStore fileStore;
        ret = &fileStore;
    } else if (k == REDIS_SESSION_KEY) {
        static TSessionRedisStore redisStore;
        ret = &redisStore;
    } else if (k == MONGODB_SESSION_KEY) {
        static TSessionMongoStore mongoStore;
        ret = &mongoStore;
    } else {
        TSessionStoreInterface *ssif = sessionStoreIfMap()->value(k);
        if (ssif) {
            ret = ssif->create(key);
        }
    }

    return ret;
}

/*!
  Destroys the \a store, assuming it is of the \a key.
 */
void TSessionStoreFactory::destroy(const QString &key, TSessionStore *store)
{
    if (!store || key.isEmpty()) {
        return;
    }

    loadKeys();
    QString k = key.toLower();
    if (k == COOKIE_SESSION_KEY) {
        // do nothing
    } else if (k == SQLOBJECT_SESSION_KEY) {
        // do nothing
    } else if (k == FILE_SESSION_KEY) {
        // do nothing
    } else if (k == REDIS_SESSION_KEY) {
        // do nothing
    } else if (k == MONGODB_SESSION_KEY) {
        // do nothing
    } else {
        TSessionStoreInterface *ssif = sessionStoreIfMap()->value(k);
        if (ssif) {
            ssif->destroy(key, store);
        }
    }
}
