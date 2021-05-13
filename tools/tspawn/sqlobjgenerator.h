#pragma once
#include "abstractobjgenerator.h"
#include <QDir>
#include <QPair>
#include <QStringList>

class TableSchema;


class SqlObjGenerator : public AbstractObjGenerator {
public:
    SqlObjGenerator(const QString &model, const QString &table);
    ~SqlObjGenerator();
    QString generate(const QString &dstDir);
    QList<QPair<QString, QMetaType::Type>> fieldList() const;
    int primaryKeyIndex() const;
    int autoValueIndex() const;
    int lockRevisionIndex() const;
    QString model() const { return modelName; }

private:
    QString modelName;
    TableSchema *tableSch {nullptr};
};

