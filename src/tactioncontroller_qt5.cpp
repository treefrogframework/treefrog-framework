/* Copyright (c) 2012-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QJsonDocument>
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
