/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDir>
#include <QList>
#include <QHash>
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
#include "tsystemglobal.h"
#if QT_VERSION >= 0x050000
# include <QJsonArray>
# include <QJsonObject>
#endif

static QMutex mutex;
static QMap<QString, TSessionStoreInterface*> *sessIfMap = 0;


/*!
  \class TSessionStoreFactory
  \brief The TSessionStoreFactory class creates TSessionStore objects.
*/

static void cleanup()
{
    QMutexLocker locker(&mutex);

    if (sessIfMap) {
        for (QMapIterator<QString, TSessionStoreInterface*> it(*sessIfMap); it.hasNext(); )  {
            it.next();
            delete it.value();
        }
        delete sessIfMap;
        sessIfMap = 0;
    }
}

/*!
  Returns the list of valid keys, i.e.\ the available session stores.
*/
QStringList TSessionStoreFactory::keys()
{
    QMutexLocker locker(&mutex);
    QStringList ret;

    loadPlugins();
    ret << TSessionSqlObjectStore().key().toLower()
        << TSessionCookieStore().key().toLower()
        << TSessionFileStore().key().toLower()
        << TSessionRedisStore().key().toLower()
        << sessIfMap->keys();

    return ret;
}

/*!
  Creates and returns a TSessionStore object that matches the given key,
  or returns 0 if no matching session store is found.
*/
TSessionStore *TSessionStoreFactory::create(const QString &key)
{
    T_TRACEFUNC("key: %s", qPrintable(key));

    static const QString COOKIE_KEY = TSessionCookieStore().key().toLower();
    static const QString SQLOBJECT_KEY = TSessionSqlObjectStore().key().toLower();
    static const QString FILE_KEY = TSessionFileStore().key().toLower();
    static const QString REDIS_KEY = TSessionRedisStore().key().toLower();

    QMutexLocker locker(&mutex);

    loadPlugins();
    TSessionStore *ret = 0;

    QString k = key.toLower();
    if (k == COOKIE_KEY) {
        ret = new TSessionCookieStore;
    } else if (k == SQLOBJECT_KEY) {
        ret = new TSessionSqlObjectStore;
    } else if (k == FILE_KEY) {
        ret = new TSessionFileStore;
    } else if (k == REDIS_KEY) {
        ret = new TSessionRedisStore;
    } else {
        TSessionStoreInterface *ssif = sessIfMap->value(k);
        if (ssif) {
            ret = ssif->create(key);
        }
    }

    return ret;
}

/*!
  Loads session store plugins in the plugin directory.
*/
void TSessionStoreFactory::loadPlugins()
{
    if (!sessIfMap) {
        sessIfMap = new QMap<QString, TSessionStoreInterface*>();
        qAddPostRoutine(cleanup);

        QDir dir(Tf::app()->pluginPath());
        QStringList list = dir.entryList(QDir::Files);
        for (QStringListIterator i(list); i.hasNext(); ) {
            QPluginLoader loader(dir.absoluteFilePath(i.next()));

            tSystemDebug("plugin library for session store: %s", qPrintable(loader.fileName()));
            if (!loader.load()) {
                tSystemError("plugin load error: %s", qPrintable(loader.errorString()));
                continue;
            }

            TSessionStoreInterface *iface = qobject_cast<TSessionStoreInterface *>(loader.instance());
            if ( iface ) {
#if QT_VERSION >= 0x050000
                QVariantList array = loader.metaData().value("MetaData").toObject().value("Keys").toArray().toVariantList();
                for (QListIterator<QVariant> it(array); it.hasNext(); ) {
                    QString key = it.next().toString().toLower();
                    tSystemInfo("Loaded session store plugin: %s", qPrintable(key));
                    sessIfMap->insert(key, iface);
                }
#else
                QStringList keys = iface->keys();
                for (QStringListIterator j(keys); j.hasNext(); ) {
                    QString key = j.next().toLower();
                    tSystemInfo("Loaded session store plugin: %s", qPrintable(key));
                    sessIfMap->insert(key, iface);
                }
#endif
            }
        }
    }
}
