/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDir>
#include <QList>
#include <QPluginLoader>
#include <QMutex>
#include <QMutexLocker>
#include <TWebApplication>
#include <TLoggerPlugin>
#include "tloggerfactory.h"
#include "tfilelogger.h"
#include "tsystemglobal.h"
#include <QJsonArray>
#include <QJsonObject>

static QMutex mutex;
static QMap<QString, TLoggerInterface *> *lggIfMap = 0;

/*!
  \class TLoggerFactory
  \brief The TLoggerFactory class creates TLogger objects.
*/

static void cleanup()
{
    QMutexLocker locker(&mutex);

    if (lggIfMap) {
        for (QMapIterator<QString, TLoggerInterface*> it(*lggIfMap); it.hasNext(); )  {
            it.next();
            delete it.value();
        }
        delete lggIfMap;
        lggIfMap = 0;
    }
}

/*!
  Returns the list of valid keys, i.e.\ the available loggers.
*/
QStringList TLoggerFactory::keys()
{
    QMutexLocker locker(&mutex);
    QStringList ret;

    loadPlugins();
    ret << TFileLogger().key().toLower()
        << lggIfMap->keys();

    return ret;
}

/*!
  Creates and returns a TLogger object that matches the given key,
  or returns 0 if no matching logger is found.
*/
TLogger *TLoggerFactory::create(const QString &key)
{
    const QString FILE_KEY = TFileLogger().key().toLower();

    QMutexLocker locker(&mutex);

    loadPlugins();
    TLogger *logger = 0;

    QString k = key.toLower();
    if (k == FILE_KEY) {
        logger = new TFileLogger();
    } else {
        TLoggerInterface *lggif = lggIfMap->value(k);
        if (lggif) {
            logger = lggif->create(key);
        }
    }

    return logger;
}


void TLoggerFactory::loadPlugins()
{
    if (!lggIfMap) {
        lggIfMap = new QMap<QString, TLoggerInterface *>();
        qAddPostRoutine(::cleanup);

        QDir dir(Tf::app()->pluginPath());
        const QStringList lst = dir.entryList(QDir::Files);
        for (auto &plg : lst) {
            QPluginLoader loader(dir.absoluteFilePath(plg));
            tSystemDebug("plugin library for logger: %s", qPrintable(loader.fileName()));
            if (!loader.load()) {
                tSystemError("plugin load error: %s", qPrintable(loader.errorString()));
                continue;
            }

            TLoggerInterface *iface = qobject_cast<TLoggerInterface *>(loader.instance());
            if ( iface ) {
                const QVariantList array = loader.metaData().value("MetaData").toObject().value("Keys").toArray().toVariantList();
                for (auto &k : array) {
                    QString key = k.toString().toLower();
                    tSystemInfo("Loaded logger plugin: %s", qPrintable(key));
                    lggIfMap->insert(key, iface);
                }
            }
        }
    }
}


/*!
  \class TLoggerInterface
  \brief The TLoggerInterface class provides an interface to implement
  TLogger plugins.
  \sa http://doc.qt.io/qt-5/plugins-howto.html
*/

/*!
  \class TLoggerPlugin
  \brief The TLoggerPlugin class provides an abstract base for custom
  TLogger plugins. Refer to 'How to Create Qt Plugins' in the Qt
  documentation.
  \sa http://doc.qt.io/qt-5/plugins-howto.html
*/

/*!
  \fn TLoggerPlugin::TLoggerPlugin()
  Constructor.
*/

/*!
  \fn virtual QStringList TLoggerPlugin::keys() const
  Implement this function to return the list of valid keys,
  i.e.\ the loggers supported by this plugin.
*/

/*!
  \fn virtual TLogger *TLoggerPlugin::create(const QString &key)
  Implement this function to create a logger matching the name specified
  by the given key.
*/
