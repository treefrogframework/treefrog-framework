#ifndef TCACHEFACTORY_H
#define TCACHEFACTORY_H

#include <QStringList>
#include <TGlobal>

class TCacheStore;


class T_CORE_EXPORT TCacheFactory
{
public:
    static QStringList keys();
    static TCacheStore *create(const QString &key);
    static void destroy(const QString &key, TCacheStore *store);
    static QMap<QString, QVariant> defaultSettings(const QString &key);

private:
    enum Backend {
        Invalid = 0,
        SQLite,
        Redis,
        MongoDB,
    };

    static bool loadCacheKeys();
};

#endif // TCACHEFACTORY_H
