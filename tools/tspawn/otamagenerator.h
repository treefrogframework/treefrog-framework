#ifndef OTAMAGENERATOR_H
#define OTAMAGENERATOR_H

#include <QStringList>
#include <QDir>
#include <QPair>
#include <QVariant>


class OtamaGenerator
{
public:
    OtamaGenerator(const QString &view, const QList<QPair<QString, QVariant::Type>> &fields, const QList<int> pkIdxs, int autoValIdx);
    bool generate(const QString &dstDir) const;

protected:
    QStringList generateViews(const QString &dstDir) const;

private:
    QString viewName;
    QList<QPair<QString, QVariant::Type>> fieldList;
    QList<int> primaryKeyIndexs;
    int autoValueIndex;
};

#endif // OTAMAGENERATOR_H
