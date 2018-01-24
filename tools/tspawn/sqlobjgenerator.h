#ifndef SQLOBJGENERATOR_H
#define SQLOBJGENERATOR_H

#include <QStringList>
#include <QDir>
#include <QPair>
#include "abstractobjgenerator.h"

class TableSchema;


class SqlObjGenerator : public AbstractObjGenerator
{
public:
    SqlObjGenerator(const QString &model, const QString &table);
    ~SqlObjGenerator();
    QString generate(const QString &dstDir);
    QList<QPair<QString, QVariant::Type>> fieldList() const;
    int primaryKeyIndex() const;
    int autoValueIndex() const;
    int lockRevisionIndex() const;
    QString model() const { return modelName; }

private:
    QString modelName;
    TableSchema *tableSch {nullptr};
};

#endif // SQLOBJGENERATOR_H
