#pragma once
#include <QDir>
#include <QPair>
#include <QString>
#include <QVariant>


class ApiControllerGenerator {
public:
    ApiControllerGenerator(const QString &controller, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx);
    ~ApiControllerGenerator() { }
    bool generate(const QString &dstDir) const;

private:
    QString _controllerName;
    QString _tableName;
    //QStringList _actionList;
    QList<QPair<QString, QMetaType::Type>> _fieldList;
    int _pkIndex {0};
};
