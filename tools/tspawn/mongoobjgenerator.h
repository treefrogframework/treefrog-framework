#pragma once
#include "abstractobjgenerator.h"
#include <QDir>
#include <QList>
#include <QPair>
#include <QString>
#include <QVariant>


class MongoObjGenerator : public AbstractObjGenerator {
public:
    MongoObjGenerator(const QString &model);

    QString generate(const QString &dstDir);
    bool createMongoObject(const QString &path);
    bool updateMongoObject(const QString &path);
    QString model() const { return modelName; }
    QList<QPair<QString, QMetaType::Type>> fieldList() const { return fields; }
    int primaryKeyIndex() const;
    int autoValueIndex() const;
    int lockRevisionIndex() const;

protected:
    QString mongoObjectFilePath(const QString &dstDir) const;

private:
    QString modelName;
    QString collectionName;
    QList<QPair<QString, QMetaType::Type>> fields;
};
