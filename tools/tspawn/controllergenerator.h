#ifndef CONTROLLERGENERATOR_H
#define CONTROLLERGENERATOR_H

#include <QString>
#include <QDir>


class ControllerGenerator
{
public:
    ControllerGenerator(const QString &controller, const QList<QPair<QString, QVariant::Type>> &fields, const QList<int> pkIdxs, int lockRevIdx);
    ControllerGenerator(const QString &controller, const QStringList &actions);
    ~ControllerGenerator() { }
    bool generate(const QString &dstDir) const;

private:
    QString controllerName;
    QString tableName;
    QStringList actionList;
    QList<QPair<QString, QVariant::Type>> fieldList;
    QList<int> primaryKeyIndexs;
    int lockRevIndex;
};

#endif // CONTROLLERGENERATOR_H
