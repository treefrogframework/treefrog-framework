#pragma once
#include <QDir>
#include <QPair>
#include <QString>
#include <QVariant>


class ControllerGenerator {
public:
    ControllerGenerator(const QString &controller, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int lockRevIdx);
    ControllerGenerator(const QString &controller, const QStringList &actions);
    ~ControllerGenerator() { }
    bool generate(const QString &dstDir) const;

private:
    static QString generateIdString(const QString &id, int type);

    QString controllerName;
    QString tableName;
    QStringList actionList;
    QList<QPair<QString, QMetaType::Type>> fieldList;
    int primaryKeyIndex {0};
    int lockRevIndex {0};

    friend class ApiControllerGenerator;
};
