#pragma once
#include "global.h"
#include "generator.h"
#include <QDir>
#include <QPair>
#include <QStringList>
#include <QVariant>


class ViteVueGenerator : public Generator {
public:
    ViteVueGenerator(const QString &view, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int autoValIdx);
    virtual ~ViteVueGenerator() {}

    bool generate(const QString &dstDir) const;

private:
    QString _viewName;
    QList<QPair<QString, QMetaType::Type>> _fieldList;
    int _primaryKeyIndex {0};
    int _autoValueIndex {0};
};
