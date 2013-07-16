/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDir>
#include <QList>
#include <QHash>
#include <QPluginLoader>
#include <QMutex>
#include <QMutexLocker>
#include <TWebApplication>
#include <TSessionStorePlugin>
#include "tsessionstorefactory.h"
#include "tsessionsqlobjectstore.h"
#include "tsessioncookiestore.h"
#include "tsessionfilestore.h"
#include "tsystemglobal.h"

static QMutex mutex;
static QHash<QString, int> hash;
static QList<TSessionStoreInterface *> *ssifs = 0;

/*!
  \class TSessionStoreFactory
  \brief The TSessionStoreFactory class creates TSessionStore objects.
*/

static void cleanup()
{
    QMutexLocker locker(&mutex);

    if (ssifs)
        delete ssifs;
    ssifs = 0;
}

/*!
  Returns the list of valid keys, i.e.\ the available session stores.
*/
QStringList TSessionStoreFactory::keys()
{
    QMutexLocker locker(&mutex);

    loadPlugins();
    QStringList ret;
    ret << TSessionSqlObjectStore().key()
        << TSessionCookieStore().key()
        << TSessionFileStore().key();

    for (QListIterator<TSessionStoreInterface *> i(*ssifs); i.hasNext(); ) {
        ret << i.next()->keys();
    }
    return ret;
}

/*!
  Creates and returns a TSessionStore object that matches the given key,
  or returns 0 if no matching session store is found.
*/
TSessionStore *TSessionStoreFactory::create(const QString &key)
{
    T_TRACEFUNC("key: %s", qPrintable(key));

    QMutexLocker locker(&mutex);
 
    loadPlugins();
    TSessionStore *ret = 0;
    QString k = key.toLower();
    int type = hash.value(k, Invalid);
    switch (type) {
    case SqlObject:
        ret = new TSessionSqlObjectStore;
        break;

    case Cookie:
        ret = new TSessionCookieStore;
        break;

    case File:
        ret = new TSessionFileStore;
        break;

    case Plugin: {
        for (QListIterator<TSessionStoreInterface *> i(*ssifs); i.hasNext(); ) {
             TSessionStoreInterface *p = i.next();
             if (p->keys().contains(k)) {
                 ret = p->create(k);
                 break;
             }
         }
         break; }

    default:
        // do nothing
        break;
    }

    return ret;
}

/*!
  Loads session store plugins in the plugin directory.
*/
void TSessionStoreFactory::loadPlugins()
{
    if (!ssifs) {
        ssifs = new QList<TSessionStoreInterface *>();
        qAddPostRoutine(::cleanup);
        
        // Init hash
        hash.insert(TSessionSqlObjectStore().key().toLower(), SqlObject);
        hash.insert(TSessionCookieStore().key().toLower(), Cookie);
        hash.insert(TSessionFileStore().key().toLower(), File);

        QDir dir(Tf::app()->pluginPath());
        QStringList list = dir.entryList(QDir::Files);
        for (QStringListIterator i(list); i.hasNext(); ) {
            QPluginLoader loader(dir.absoluteFilePath(i.next()));
            TSessionStoreInterface *iface = qobject_cast<TSessionStoreInterface *>(loader.instance());
            if ( iface ) {
                ssifs->append(iface);
                QStringList keys = iface->keys();
                for (QStringListIterator j(keys); j.hasNext(); ) {
                    hash.insert(j.next(), Plugin);
                }
            }   
        }
    }
}
