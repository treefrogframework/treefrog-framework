/* Copyright (c) 2012-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <TActionController>

/*!
  Renders the JSON document \a document as HTTP response.
  This is available on Qt 5.
*/
bool TActionController::renderJson(const QJsonDocument &document)
{
    return sendData(document.toJson(
#if QT_VERSION >= 0x050100
                        QJsonDocument::Compact
#endif
                        ), "application/json; charset=utf-8");
}

/*!
  Renders the JSON object \a object as HTTP response.
  This is available on Qt 5.
*/
bool TActionController::renderJson(const QJsonObject &object)
{
    return renderJson(QJsonDocument(object));
}

/*!
  Renders the JSON array \a array as HTTP response.
  This is available on Qt 5.
*/
bool TActionController::renderJson(const QJsonArray &array)
{
    return renderJson(QJsonDocument(array));
}

/*!
  Renders the \a map as a JSON object.
  This is available on Qt 5.
*/
bool TActionController::renderJson(const QVariantMap &map)
{
    return renderJson(QJsonObject::fromVariantMap(map));
}

/*!
  Renders the \a list as a JSON array.
  This is available on Qt 5.
*/
bool TActionController::renderJson(const QVariantList &list)
{
    return renderJson(QJsonArray::fromVariantList(list));
}

/*!
  Renders the \a list as a JSON array.
  This is available on Qt 5.
*/
bool TActionController::renderJson(const QStringList &list)
{
    return renderJson(QJsonArray::fromStringList(list));
}

#if QT_VERSION >= 0x050c00  // 5.12.0
bool TActionController::renderCbor(const QVariant &variant)
{
    return renderCbor(QCborValue::fromVariant(variant));
}

bool TActionController::renderCbor(const QVariantMap &map)
{
    return renderCbor(QCborMap::fromVariantMap(map));
}

bool TActionController::renderCbor(const QVariantHash &hash)
{
    return renderCbor(QCborMap::fromVariantHash(hash));
}

bool TActionController::renderCbor(const QCborValue &value)
{
    QCborValue val = value;
    return sendData(val.toCbor(), "application/cbor");
}

bool TActionController::renderCbor(const QCborMap &map)
{
    return renderCbor(map.toCborValue());
}

bool TActionController::renderCbor(const QCborArray &array)
{
    return renderCbor(array.toCborValue());
}

#endif
