/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tloggerfactory.h"
#include "tfilelogger.h"
#include "tsystemglobal.h"
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QPluginLoader>
#include <TLoggerPlugin>
#include <TWebApplication>

namespace {
void cleanup();
QString FILE_LOGGER_KEY;


void loadKeys()
{
    static bool done = []() {
        // Constants
        FILE_LOGGER_KEY = TFileLogger().key().toLower();
        return true;
    }();
    Q_UNUSED(done);
}


QMap<QString, TLoggerInterface *> *loggerIfMap()
{
    static QMap<QString, TLoggerInterface *> *lggIfMap = []() {
        auto lggIfMap = new QMap<QString, TLoggerInterface *>();
        qAddPostRoutine(cleanup);

        QDir dir(Tf::app()->pluginPath());
        const QStringList lst = dir.entryList(QDir::Files);
        for (auto &plg : lst) {
            QPluginLoader loader(dir.absoluteFilePath(plg));
            tSystemDebug("plugin library for logger: %s", qUtf8Printable(loader.fileName()));
            if (!loader.load()) {
                tSystemError("plugin load error: %s", qUtf8Printable(loader.errorString()));
                continue;
            }

            TLoggerInterface *iface = dynamic_cast<TLoggerInterface *>(loader.instance());
            if (iface) {
                const QVariantList array = loader.metaData().value("MetaData").toObject().value("Keys").toArray().toVariantList();
                for (auto &k : array) {
                    QString key = k.toString().toLower();
                    tSystemInfo("Loaded logger plugin: %s", qUtf8Printable(key));
                    lggIfMap->insert(key, iface);
                }
            }
        }
        return lggIfMap;
    }();
    return lggIfMap;
}


void cleanup()
{
    auto lggIfMap = loggerIfMap();
    if (lggIfMap) {
        for (auto it = lggIfMap->begin(); it != lggIfMap->end(); ++it) {
            delete it.value();
        }
        delete lggIfMap;
    }
}
}

/*!
  \class TLoggerFactory
  \brief The TLoggerFactory class creates TLogger objects.
*/

/*!
  Returns the list of valid keys, i.e.\ the available loggers.
*/
QStringList TLoggerFactory::keys()
{
    QStringList ret;

    loadKeys();
    ret << FILE_LOGGER_KEY
        << loggerIfMap()->keys();

    return ret;
}

/*!
  Creates and returns a TLogger object that matches the given key,
  or returns 0 if no matching logger is found.
*/
TLogger *TLoggerFactory::create(const QString &key)
{
    TLogger *logger = nullptr;

    loadKeys();
    QString k = key.toLower();
    if (k == FILE_LOGGER_KEY) {
        logger = new TFileLogger();
    } else {
        TLoggerInterface *lggif = loggerIfMap()->value(k);
        if (lggif) {
            logger = lggif->create(key);
        }
    }
    return logger;
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
