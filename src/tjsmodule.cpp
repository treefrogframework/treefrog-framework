/* Copyright (c) 2016-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsystemglobal.h"
#include <QJSEngine>
#include <QJSValue>
#include <TJSInstance>
#include <TJSLoader>
#include <TJSModule>

//#define tSystemError(fmt, ...)  printf(fmt "\n", ## __VA_ARGS__)
//#define tSystemDebug(fmt, ...)  printf(fmt "\n", ## __VA_ARGS__)

namespace {
inline const char *prop(const QJSValue &val, const QString &name = QString())
{
    return (name.isEmpty()) ? qUtf8Printable(val.toString()) : qUtf8Printable(val.property(name).toString());
}
}

/*!
  \class TJSModule
  \brief The TJSModule class represents a module for evaluating JavaScript code.
  \see https://doc.qt.io/qt-6/qjsengine.html
*/


TJSModule::TJSModule(QObject *parent) :
    QObject(parent), _jsEngine(new QJSEngine()), _loadedFiles(), _funcObj(nullptr),
    _lastFunc()
{
    _jsEngine->installExtensions(QJSEngine::ConsoleExtension);
    _jsEngine->evaluate("exports={};module={};module.exports={};");
}


TJSModule::~TJSModule()
{
    delete _funcObj;
    delete _jsEngine;
}


QJSValue TJSModule::evaluate(const QString &program, const QString &fileName, int lineNumber)
{
    QMutexLocker locker(&_mutex);

    QJSValue ret = _jsEngine->evaluate(program, fileName, lineNumber);
    if (ret.isError()) {
        tSystemError("JS uncaught exception at %s:%s : %s", prop(ret, "fileName"),
            prop(ret, "lineNumber"), prop(ret, "message"));
    }
    return ret;
}


QJSValue TJSModule::call(const QString &func, const QJSValue &arg)
{
    QJSValueList args = {arg};
    return call(func, args);
}


QJSValue TJSModule::call(const QString &func, const QJSValueList &args)
{
    QMutexLocker locker(&_mutex);
    QJSValue ret;

    QString funcsym = QString::number(args.count()) + func;
    if (funcsym != _lastFunc || !_funcObj) {
        _lastFunc = funcsym;

        QString argstr;
        for (int i = 0; i < args.count(); i++) {
            argstr = QChar('a') + QString::number(i) + ',';
        }
        argstr.chop(1);

        QString defFunc = QString("(function(%1){return(%2(%1));})").arg(argstr, func);

        if (!_funcObj) {
            _funcObj = new QJSValue();
        }

        *_funcObj = evaluate(defFunc);
        if (_funcObj->isError()) {
            goto eval_error;
        }
    }

    ret = _funcObj->call(args);
    if (ret.isError()) {
        tSystemError("JS uncaught exception at %s:%s : %s", prop(ret, "fileName"),
            prop(ret, "lineNumber"), prop(ret));
        goto eval_error;
    }

    return ret;

eval_error:
    delete _funcObj;
    _funcObj = nullptr;
    return ret;
}


TJSInstance TJSModule::callAsConstructor(const QString &constructorName, const QJSValue &arg)
{
    QJSValueList args = {arg};
    return callAsConstructor(constructorName, args);
}


TJSInstance TJSModule::callAsConstructor(const QString &constructorName, const QJSValueList &args)
{
    QMutexLocker locker(&_mutex);

    QJSValue construct = evaluate(constructorName);
    tSystemDebug("construct: %s", qUtf8Printable(construct.toString()));
    QJSValue res = construct.callAsConstructor(args);
    if (res.isError()) {
        tSystemError("JS uncaught exception at %s:%s : %s", prop(res, "fileName"),
            prop(res, "lineNumber"), prop(res));
    }
    return TJSInstance(res);
}


QJSValue TJSModule::import(const QString &moduleName)
{
    TJSLoader loader(moduleName);
    return loader.importTo(this, false);
}


QJSValue TJSModule::import(const QString &defaultMember, const QString &moduleName)
{
    TJSLoader loader(defaultMember, moduleName);
    return loader.importTo(this, false);
}
