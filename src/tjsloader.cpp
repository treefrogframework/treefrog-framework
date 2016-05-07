/* Copyright (c) 2016, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tjsloader.h"

static QMap<QString, TJSContext*> jsContexts;
static QMutex mutex(QMutex::Recursive);

/*!
  Constructor.
 */
TJSLoader::TJSLoader()
{ }

/*!
  Loads the JavaScript module \a moduleName and returns true if
  successful; otherwise returns false.
*/
TJSContext *TJSLoader::loadJSModule(const QString &moduleName)
{
    return loadJSModule(QString(), moduleName);
}

/*!
  Loads the JavaScript module \a moduleName and returns the
  JavaScript context if successful; otherwise returns null
  pointer.
*/
TJSContext *TJSLoader::loadJSModule(const QString &defaultMember, const QString &moduleName)
{
    QMutexLocker lock(&mutex);
    QString key = defaultMember + QLatin1Char(';') + moduleName;
    TJSContext *context = jsContexts.value(key);

    if (!context) {
        context = new TJSContext();
        QJSValue res;
        if (defaultMember.isEmpty()) {
            res = context->import(moduleName);
        } else {
            res = context->import(defaultMember, moduleName);
        }

        if (res.isError()) {
            delete context;
            context = nullptr;
        } else {
            jsContexts.insert(key, context);
        }
    }
    return context;
}


TJSInstance TJSLoader::loadJSModuleAsConstructor(const QString &moduleName, const QJSValue &arg)
{
    QJSValueList args = { arg };
    return loadJSModuleAsConstructor(moduleName, args);
}


TJSInstance TJSLoader::loadJSModuleAsConstructor(const QString &moduleName, const QJSValueList &args)
{
    QMutexLocker lock(&mutex);
    QString constructorName = QLatin1String("_T") + QFileInfo(moduleName).baseName().replace(QChar('-'), QChar('_'));
    auto *ctx = loadJSModule(constructorName, moduleName);
    return (ctx) ? ctx->callAsConstructor(constructorName, args) : TJSInstance();
}
