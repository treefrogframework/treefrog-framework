#pragma once
#include <QStringList>
#include <TGlobal>

class TLogger;


class T_CORE_EXPORT TLoggerFactory {
public:
    static QStringList keys();
    static TLogger *create(const QString &key);
};

