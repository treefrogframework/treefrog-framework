/* Copyright (c) 2012-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <TActionController>


/*!
  Sends the JSON document \a document as HTTP response.
  This is available on Qt 5.
*/
bool TActionController::renderJson(const QJsonDocument &document)
{
    return sendData(document.toJson(), "application/json");
}

/*!
  Sends the JSON object \a object as HTTP response.
  This is available on Qt 5.
*/
bool TActionController::renderJson(const QJsonObject &object)
{
    return renderJson(QJsonDocument(object));
}

/*!
  Sends the JSON array \a array as HTTP response.
  This is available on Qt 5.
*/
bool TActionController::renderJson(const QJsonArray &array)
{
    return renderJson(QJsonDocument(array));
}

/*!
  Sends the \a map as a JSON object.
  This is available on Qt 5.
*/
bool TActionController::renderJson(const QVariantMap &map)
{
    return renderJson(QJsonObject::fromVariantMap(map));
}

/*!
  Sends the \a list as a JSON array.
  This is available on Qt 5.
*/
bool TActionController::renderJson(const QVariantList &list)
{
    return renderJson(QJsonArray::fromVariantList(list));
}

/*!
  Sends the \a list as a JSON array.
  This is available on Qt 5.
*/
bool TActionController::renderJson(const QStringList &list)
{
    return renderJson(QJsonArray::fromStringList(list));
}
