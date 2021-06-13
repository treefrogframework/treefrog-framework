#pragma once
#include <QDir>
#include <QPair>
#include <QString>
#include <QVariant>


class ApiServiceGenerator {
public:
    ApiServiceGenerator(const QString &service, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx);
    ~ApiServiceGenerator() { }
    bool generate(const QString &dstDir) const;

private:
    QString _serviceName;
    QString _tableName;
    QList<QPair<QString, QMetaType::Type>> _fieldList;
    int _pkIndex {0};
    int _lockRevIndex {0};
};

