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
*/
bool TActionController::sendJson(const QJsonDocument &document)
{
    return sendData(document.toJson(), "application/json");
}

/*!
  Sends the JSON object \a object as HTTP response.
*/
bool TActionController::sendJson(const QJsonObject &object)
{
    return sendJson(QJsonDocument(object));
}

/*!
  Sends the JSON array \a array as HTTP response.
*/
bool TActionController::sendJson(const QJsonArray &array)
{
    return sendJson(QJsonDocument(array));
}

/*!
  Sends the \a map as a JSON object.
*/
bool TActionController::sendJson(const QVariantMap &map)
{
    return sendJson(QJsonObject::fromVariantMap(map));
}

/*!
  Sends the \a list as a JSON array.
*/
bool TActionController::sendJson(const QVariantList &list)
{
    return sendJson(QJsonArray::fromVariantList(list));
}

/*!
  Sends the \a list as a JSON array.
*/
bool TActionController::sendJson(const QStringList &list)
{
    return sendJson(QJsonArray::fromStringList(list));
}
