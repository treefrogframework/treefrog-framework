/* Copyright (c) 2016-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsystemglobal.h"
#include <QJSValueIterator>
#include <TJSInstance>

/*!
  \class TJSInstance
  \brief The TJSInstance class acts as a instance for JavaScript data types.
  \see https://doc.qt.io/qt-6/qjsvalue.html
*/


TJSInstance::TJSInstance() :
    QJSValue()
{
}


TJSInstance::TJSInstance(const TJSInstance &other) :
    QJSValue(other)
{
}


TJSInstance::TJSInstance(const QJSValue &other) :
    QJSValue(other)
{
}


TJSInstance::~TJSInstance()
{
}


QJSValue TJSInstance::call(const QString &method, const QJSValue &arg)
{
    QJSValueList args = {arg};
    return call(method, args);
}


QJSValue TJSInstance::call(const QString &method, const QJSValueList &args)
{
    if (isError()) {
        tSystemError("Uncaught exception at line {} : {}", property("lineNumber").toInt(),
            toString());
        return QJSValue();
    }

    auto meth = property(method);
    return (meth.isError()) ? meth : meth.callWithInstance(*dynamic_cast<QJSValue *>(this), args);
}
