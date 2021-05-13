#pragma once
#include <QDir>
#include <QPair>
#include <QStringList>
#include <QVariant>


class OtamaGenerator {
public:
    OtamaGenerator(const QString &view, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int autoValIdx);
    bool generate(const QString &dstDir) const;

protected:
    QStringList generateViews(const QString &dstDir) const;

private:
    QString viewName;
    QList<QPair<QString, QMetaType::Type>> fieldList;
    int primaryKeyIndex {0};
    int autoValueIndex {0};
};

