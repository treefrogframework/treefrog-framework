/* Copyright (c) 2010, AOYAMA Kazuharu
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
#include <TMailerPlugin>
#include "tmailerfactory.h"
#include "tsmtpmailer.h"


static QMutex mutex;
static QHash<QString, int> hash;
static QList<TMailerInterface *> *ssifs = 0;

static void cleanup()
{
    QMutexLocker locker(&mutex);

    if (ssifs)
        delete ssifs;
    ssifs = 0;
}


QStringList TMailerFactory::keys()
{
    QMutexLocker locker(&mutex);

    loadPlugins();
    QStringList ret;
    ret << TSmtpMailer().key();

    for (QListIterator<TMailerInterface *> i(*ssifs); i.hasNext(); ) {
        ret << i.next()->keys();
    }
    return ret;
}


TMailer *TMailerFactory::create(const QString &key)
{
    QMutexLocker locker(&mutex);

    loadPlugins();
    TMailer *ret = 0;
    QString k = key.toLower();
    int type = hash.value(k, Invalid);
    switch (type) {
    case Smtp:
        ret = new TSmtpMailer;
        break;

    case Plugin: {
        for (QListIterator<TMailerInterface *> i(*ssifs); i.hasNext(); ) {
             TMailerInterface *p = i.next();
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


void TMailerFactory::loadPlugins()
{
    if (!ssifs) {
        ssifs = new QList<TMailerInterface *>();
        qAddPostRoutine(cleanup);

        // Init hash
        hash.insert(TSmtpMailer().key().toLower(), Smtp);

        QDir dir(Tf::app()->pluginsPath());
        QStringList list = dir.entryList(QDir::Files);
        for (QStringListIterator i(list); i.hasNext(); ) {
            QPluginLoader loader(dir.absoluteFilePath(i.next()));
            TMailerInterface *iface = qobject_cast<TMailerInterface *>(loader.instance());
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
