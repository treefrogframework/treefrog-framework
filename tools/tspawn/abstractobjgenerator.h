#ifndef ABSTRACTOBJGENERATOR_H
#define ABSTRACTOBJGENERATOR_H

#include <QString>
#include <QList>
#include <QPair>
#include <QVariant>


class AbstractObjGenerator
{
public:
    virtual ~AbstractObjGenerator() { }
    virtual QString generate(const QString &dstDir) = 0;
    virtual QList<QPair<QString, QVariant::Type>> fieldList() const = 0;
    virtual int primaryKeyIndex() const { return -1; }
    virtual int autoValueIndex() const { return -1; }
    virtual int lockRevisionIndex() const { return -1; }
};

#endif // ABSTRACTOBJGENERATOR_H
