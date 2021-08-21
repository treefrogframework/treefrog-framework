#pragma once
#include "global.h"
#include <QDir>
#include <QPair>
#include <QStringList>
#include <QVariant>


class ErbGenerator {
public:
    ErbGenerator(const QString &view, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int autoValIdx);
    virtual ~ErbGenerator() {}
    bool generate(const QString &dstDir) const;

protected:
    virtual QString indexTemplate() const;
    virtual QString showTemplate() const;
    virtual QString createTemplate() const;
    virtual QString saveTemplate() const;
    virtual PlaceholderList replaceList() const;

    QString _viewName;
    QList<QPair<QString, QMetaType::Type>> _fieldList;
    int _primaryKeyIndex {0};
    int _autoValueIndex {0};
};
