/* Copyright (c) 2016, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TJSInstance>
#include <QJSValueIterator>


TJSInstance::TJSInstance() : QJSValue()
{ }


TJSInstance::TJSInstance(const TJSInstance &other)
    : QJSValue(other)
{ }


TJSInstance::TJSInstance(const QJSValue &other)
    : QJSValue(other)
{ }


TJSInstance::~TJSInstance()
{ }


QJSValue TJSInstance::call(const QString &method, const QJSValue &arg)
{
    QJSValueList args = { arg };
    return call(method, args);
}


QJSValue TJSInstance::call(const QString &method, const QJSValueList &args)
{
    auto meth = property(method);
    return (meth.isError()) ? meth : meth.callWithInstance(*dynamic_cast<QJSValue *>(this), args);
}
