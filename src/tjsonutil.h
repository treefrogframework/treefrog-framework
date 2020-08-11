#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QVariantMap>


template <class T>
inline QJsonArray tfModelListToJsonArray(const QList<T> &models)
{
    QJsonArray array;

    for (auto &mdl : models) {
        array.append(QJsonValue(QJsonObject::fromVariantMap(mdl.toVariantMap())));
    }
    return array;
}

