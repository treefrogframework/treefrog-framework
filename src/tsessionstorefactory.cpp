/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDir>
#include <QList>
#include <QMapIterator>
#include <QPluginLoader>
#include <QMutex>
#include <QMutexLocker>
#include <TWebApplication>
#include <TSessionStorePlugin>
#include "tsessionstorefactory.h"
#include "tsessionsqlobjectstore.h"
#include "tsessioncookiestore.h"
#include "tsessionfilestore.h"
#include "tsessionredisstore.h"
#include "tsessionmongostore.h"
#include "tsystemglobal.h"
#include <QJsonArray>
#include <QJsonObject>

static void cleanup();

static QString COOKIE_SESSION_KEY;
static QString SQLOBJECT_SESSION_KEY;
static QString FILE_SESSION_KEY;
static QString REDIS_SESSION_KEY;
static QString MONGODB_SESSION_KEY;


static void loadKeys()
{
    static bool done = []() {
        // Constants
        COOKIE_SESSION_KEY    = TSessionCookieStore().key().toLower();
        SQLOBJECT_SESSION_KEY = TSessionSqlObjectStore().key().toLower();
        FILE_SESSION_KEY      = TSessionFileStore().key().toLower();
        REDIS_SESSION_KEY     = TSessionRedisStore().key().toLower();
        MONGODB_SESSION_KEY   = TSessionMongoStore().key().toLower();
        return true;
    }();
    Q_UNUSED(done);
}


/*!
  \class TSessionStoreFactory
  \brief The TSessionStoreFactory class creates TSessionStore objects.
*/

/*!
  Loads session store plugins in the plugin directory and returns a pointer to QMap instance.
*/
static QMap<QString, TSessionStoreInterface*> *sessionStoreIfMap()
{
    static QMap<QString, TSessionStoreInterface*> *sessIfMap = []() {
        auto sessIfMap = new QMap<QString, TSessionStoreInterface*>();
        qAddPostRoutine(cleanup);

        QDir dir(Tf::app()->pluginPath());
        const QStringList list = dir.entryList(QDir::Files);
        for (auto &file : list) {
            QPluginLoader loader(dir.absoluteFilePath(file));

            tSystemDebug("plugin library for session store: %s", qPrintable(loader.fileName()));
            if (!loader.load()) {
                tSystemError("plugin load error: %s", qPrintable(loader.errorString()));
                continue;
            }

            TSessionStoreInterface *iface = qobject_cast<TSessionStoreInterface *>(loader.instance());
            if ( iface ) {
                const QVariantList array = loader.metaData().value("MetaData").toObject().value("Keys").toArray().toVariantList();
                for (auto &k : array) {
                    QString key = k.toString().toLower();
                    tSystemInfo("Loaded session store plugin: %s", qPrintable(key));
                    sessIfMap->insert(key, iface);
                }
            }
        }
        return sessIfMap;
    }();
    return sessIfMap;
}


static void cleanup()
{
    auto sessIfMap = sessionStoreIfMap();
    if (sessIfMap) {
        for (QMapIterator<QString, TSessionStoreInterface*> it(*sessIfMap); it.hasNext(); )  {
            it.next();
            delete it.value();
        }
        delete sessIfMap;
    }
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
    if (!store) {
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
