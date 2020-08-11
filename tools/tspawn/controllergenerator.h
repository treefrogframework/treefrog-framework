#pragma once
#include <QDir>
#include <QPair>
#include <QString>
#include <QVariant>


class ControllerGenerator {
public:
    ControllerGenerator(const QString &controller, const QList<QPair<QString, QVariant::Type>> &fields, int pkIdx, int lockRevIdx);
    ControllerGenerator(const QString &controller, const QStringList &actions);
    ~ControllerGenerator() { }
    bool generate(const QString &dstDir) const;

private:
    QString controllerName;
    QString tableName;
    QStringList actionList;
    QList<QPair<QString, QVariant::Type>> fieldList;
    int primaryKeyIndex {0};
    int lockRevIndex {0};
};

