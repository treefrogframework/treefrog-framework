/* Copyright (c) 2016, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <atomic>
#include <QJSEngine>
#include <QJSValue>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <TWebApplication>
#include <TJSContext>
#include <TReactComponent>
#include "tsystemglobal.h"


TReactComponent::TReactComponent(const QString &script)
    : context(new TJSContext()), jsValue(new QJSValue()), scriptPath(), loadedTime()
{
    init();
    load(script);
}


// TReactComponent::TReactComponent(const QStringList &scripts)
//     : context(new TJSContext()), jsValue(new QJSValue())
// {
//     init();
//     for (auto &s : scripts) {
//         load(s);
//     }
// }


TReactComponent::~TReactComponent()
{
    delete jsValue;
    delete context;
}


void TReactComponent::init()
{
    load(Tf::app()->webRootPath()+ "script" + QDir::separator() + "react.min.js");
    load(Tf::app()->webRootPath()+ "script" + QDir::separator() + "react-dom-server.min.js");
}


QString TReactComponent::filePath() const
{
    return scriptPath;
}


bool TReactComponent::load(const QString &scriptFile)
{
    bool ok;
    if (QFileInfo(scriptFile).suffix().compare("jsx", Qt::CaseInsensitive) == 0) {
        // Loads JSX file
        QString program = compileJsxFile(scriptFile);
        QJSValue res = context->evaluate(program, scriptFile);
        ok = !res.isError();
    } else {
        ok = context->load(scriptFile);
    }

    if (ok) {
        loadedTime = QDateTime::currentDateTime();
        scriptPath = scriptFile;
    } else {
        loadedTime = QDateTime();
        scriptPath = QString();
    }
    return ok;
}


QString TReactComponent::renderToString(const QString &component)
{
    QString comp = component.trimmed();
    if (!comp.startsWith("<")) {
        comp.prepend('<');
    }
    if (!comp.endsWith("/>")) {
        comp.append("/>");
    }

    QString func = QLatin1String("ReactDOMServer.renderToString(") + compileJsx(comp) + QLatin1String(");");
    return context->evaluate(func).toString();
}


QString TReactComponent::compileJsx(const QString &jsx)
{
    static TJSContext js;
    static std::atomic<bool> once(false);
    static QMutex mutex;

    if (!once.load()) {
        QMutexLocker locker(&mutex);
        if (!once.load()) {
            js.load(Tf::app()->webRootPath()+ "script" + QDir::separator() + "JSXTransformer.js");
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
        return false;
    }

    QTextStream stream(&script);
    QString contents = stream.readAll();
    script.close();
    return compileJsx(contents);
}
