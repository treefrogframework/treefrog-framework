#ifndef ERBGENERATOR_H
#define ERBGENERATOR_H

#include <QStringList>
#include <QDir>
#include <QPair>
#include <QVariant>


class ErbGenerator
{
public:
    ErbGenerator(const QString &view, const QList<QPair<QString, QVariant::Type>> &fields,const QList<int> pkIdxs, int autoValIdx);
    bool generate(const QString &dstDir) const;

private:
    QString viewName;
    QList<QPair<QString, QVariant::Type>> fieldList;
    QList<int> primaryKeyIndexs;
    int autoValueIndex;
};

#endif // ERBGENERATOR_H
