/* Copyright (c) 2016, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QJSEngine>
#include <QJSValue>
#include <QFile>
#include <QTextStream>
#include "tjscontext.h"
#include "tsystemglobal.h"


inline const char *prop(const QJSValue &val, const QString &name = QString())
{
    return (name.isEmpty()) ? qPrintable(val.toString()) : qPrintable(val.property(name).toString());
}


TJSContext::TJSContext(const QStringList &scriptFiles)
    : jsEngine(new QJSEngine()), funcObj(nullptr), lastFunc(), mutex(QMutex::Recursive)
{
    for (auto &file : scriptFiles) {
        load(file);
    }
}


TJSContext::~TJSContext()
{
    if (funcObj) {
        delete funcObj;
    }
    delete jsEngine;
}


QJSValue TJSContext::evaluate(const QString &program, const QString &fileName, int lineNumber)
{
    QJSValue ret = jsEngine->evaluate(program, fileName, lineNumber);
    if (ret.isError()) {
        tSystemError("JS uncaught exception at %s:%s : %s", prop(ret, "fileName"),
                     prop(ret, "lineNumber"), prop(ret, "message"));
    }
    return ret;
}


QJSValue TJSContext::call(const QString &func, const QJSValue &arg)
{
    QJSValueList args = { arg };
    return call(func, args);
}


QJSValue TJSContext::call(const QString &func, const QJSValueList &args)
{
    QMutexLocker locker(&mutex);
    QJSValue ret;

    QString funcsym = QString::number(args.count()) + func;
    if (funcsym != lastFunc || !funcObj) {
        lastFunc = funcsym;

        QString argstr;
        for (int i = 0; i < args.count(); i++) {
            argstr = QChar('a') + QString::number(i) + ',';
        }
        argstr.chop(1);

        QString defFunc = QString("function(%1){return(%2(%1));}").arg(argstr, func);

        if (!funcObj) {
            funcObj = new QJSValue();
        }

        *funcObj = evaluate(defFunc);
        if (funcObj->isError()) {
            goto eval_error;
        }
    }

    ret = funcObj->call(args);
    if (ret.isError()) {
        tSystemError("JS uncaught exception at %s:%s : %s", prop(ret, "fileName"),
                     prop(ret, "lineNumber"), prop(ret));
        goto eval_error;
    }

    return ret;

eval_error:
    delete funcObj;
    funcObj = nullptr;
    return ret;
}


bool TJSContext::load(const QString &fileName)
{
    QMutexLocker locker(&mutex);

    QFile script(fileName);
    if (!script.open(QIODevice::ReadOnly)) {
        // open error
        tSystemError("TJSContext open error: %s", qPrintable(fileName));
        return false;
    }

    QTextStream stream(&script);
    QString contents = stream.readAll();
    script.close();

    QJSValue res = evaluate(contents, fileName);
    if (res.isError()) {
        return false;
    }

    tSystemDebug("TJSContext evaluation completed: %s", qPrintable(fileName));
    return true;
}
