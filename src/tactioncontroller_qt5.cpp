/* Copyright (c) 2012-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtCore>
#include <TActionController>

/*!
  Renders the JSON document \a document as HTTP response.
  This is available on Qt 5.
*/
bool TActionController::renderJson(const QJsonDocument &document)
{
    return sendData(document.toJson(QJsonDocument::Compact), "application/json; charset=utf-8");
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

/*!
  Renders a CBOR object \a variant as HTTP response.
*/
bool TActionController::renderCbor(const QVariant &variant, QCborValue::EncodingOptions opt)
{
    return renderCbor(QCborValue::fromVariant(variant), opt);
}

/*!
  Renders a CBOR object \a map as HTTP response.
*/
bool TActionController::renderCbor(const QVariantMap &map, QCborValue::EncodingOptions opt)
{
    return renderCbor(QCborMap::fromVariantMap(map), opt);
}

/*!
  Renders a CBOR object \a hash as HTTP response.
*/
bool TActionController::renderCbor(const QVariantHash &hash, QCborValue::EncodingOptions opt)
{
    return renderCbor(QCborMap::fromVariantHash(hash), opt);
}

/*!
  Renders a CBOR \a value as HTTP response.
*/
bool TActionController::renderCbor(const QCborValue &value, QCborValue::EncodingOptions opt)
{
    QCborValue val = value;
    return sendData(val.toCbor(opt), "application/cbor");
}

/*!
  Renders a CBOR \a map as HTTP response.
*/
bool TActionController::renderCbor(const QCborMap &map, QCborValue::EncodingOptions opt)
{
    return renderCbor(map.toCborValue(), opt);
}

/*!
  Renders a CBOR \a array as HTTP response.
*/
bool TActionController::renderCbor(const QCborArray &array, QCborValue::EncodingOptions opt)
{
    return renderCbor(array.toCborValue(), opt);
}

#endif
