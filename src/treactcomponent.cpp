/* Copyright (c) 2016, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QJSEngine>
#include <QJSValue>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <TWebApplication>
#include <TJSContext>
#include <TReactComponent>
#include "tsystemglobal.h"

//#define tSystemError(fmt, ...)  printf(fmt "\n", ## __VA_ARGS__)
//#define tSystemDebug(fmt, ...)  printf(fmt "\n", ## __VA_ARGS__)


TReactComponent::TReactComponent(const QString &moduleName)
    : context(new TJSContext()), jsValue(new QJSValue()), modulePath(), loadedTime()
{
    init();
    import(moduleName);
}


TReactComponent::~TReactComponent()
{
    delete jsValue;
    delete context;
}


void TReactComponent::init()
{
    context->import("React", "react-with-addons");
    context->import("ReactDOMServer", "react-dom-server");
}


QString TReactComponent::filePath() const
{
    return modulePath;
}


bool TReactComponent::import(const QString &moduleName)
{
    return import("", moduleName);
}


bool TReactComponent::import(const QString &defaultMember, const QString &moduleName)
{
    bool ok = false;

    if (moduleName.isEmpty()) {
        return ok;
    }

    if (QFileInfo(moduleName).suffix().compare("jsx", Qt::CaseInsensitive) == 0) {
        // Loads JSX file
        QString program = compileJsxFile(moduleName);
        QJSValue res = context->evaluate(program, moduleName);
        ok = !res.isError();
    } else {
        if (defaultMember.isEmpty()) {
            ok = !context->import(moduleName).isError();
        } else {
            ok = !context->import(defaultMember, moduleName).isError();
        }
    }

    if (ok) {
        loadedTime = QDateTime::currentDateTime();
        modulePath = context->lastImportedModulePath();
    } else {
        loadedTime = QDateTime();
        modulePath = QString();
    }
    return ok;
}


QString TReactComponent::renderToString(const QString &component)
{
    QString func = QLatin1String("ReactDOMServer.renderToString(") + compileJsx(component) + QLatin1String(");");
    tSystemDebug("TReactComponent func: %s", qPrintable(func));
    return context->evaluate(func).toString();
}


QString TReactComponent::compileJsx(const QString &jsx)
{
    static TJSContext js;
    static bool once = false;
    static QMutex mutex;

    if (Q_UNLIKELY(!once)) {
        QMutexLocker locker(&mutex);
        if (!once) {
            js.import("JSXTransformer", "JSXTransformer");
            once = true;
        }
    }

    QJSValue jscode = js.call("JSXTransformer.transform", QJSValue(jsx));
    //tSystemDebug("code:%s", qPrintable(jscode.property("code").toString()));
    return jscode.property("code").toString();
}


QString TReactComponent::compileJsxFile(const QString &fileName)
{
    QFile script(fileName);
    if (!script.open(QIODevice::ReadOnly)) {
        // open error
        tSystemError("TReactComponent open error: %s", qPrintable(fileName));
        return QString();
    }

    QTextStream stream(&script);
    QString contents = stream.readAll();
    script.close();
    return compileJsx(contents);
}
