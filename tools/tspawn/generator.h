#pragma once
#include <QtCore>


class Generator {
public:
    virtual ~Generator() = default;
    virtual bool generate(const QString &dstDir) const = 0;
};
