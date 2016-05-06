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


TReactComponent::TReactComponent(const QString &script)
    : context(new TJSContext()), jsValue(new QJSValue()), scriptPath(), loadedTime()
{
    init();
    import(script);
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
    import(Tf::app()->webRootPath()+ "script" + QDir::separator() + "react.min.js");
    import(Tf::app()->webRootPath()+ "script" + QDir::separator() + "react-dom-server.min.js");
}


QString TReactComponent::filePath() const
{
    return scriptPath;
}


bool TReactComponent::import(const QString &moduleName)
{
    bool ok;
    if (QFileInfo(moduleName).suffix().compare("jsx", Qt::CaseInsensitive) == 0) {
        // Loads JSX file
        QString program = compileJsxFile(moduleName);
        QJSValue res = context->evaluate(program, moduleName);
        ok = !res.isError();
    } else {
        ok = !context->import(moduleName).isError();
    }

    if (ok) {
        loadedTime = QDateTime::currentDateTime();
        scriptPath = moduleName;
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
    static bool once = false;
    static QMutex mutex;

    if (Q_UNLIKELY(!once)) {
        QMutexLocker locker(&mutex);
        if (!once) {
            js.import(Tf::app()->webRootPath()+ "script" + QDir::separator() + "JSXTransformer.js");
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
