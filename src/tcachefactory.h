#ifndef TCACHEFACTORY_H
#define TCACHEFACTORY_H

#include <TGlobal>
#include "tcachestore.h"
#include <QStringList>


class T_CORE_EXPORT TCacheFactory
{
public:
    static QStringList keys();
    static TCacheStore *create(const QString &key);
    static void destroy(const QString &key, TCacheStore *store);
    static QMap<QString, QVariant> defaultSettings(const QString &key);
    static TCacheStore::DbType dbType(const QString &key);

private:
    static bool loadCacheKeys();
};

#endif // TCACHEFACTORY_H
