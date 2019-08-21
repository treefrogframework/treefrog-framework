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

private:
    enum Backend {
        Invalid = 0,
        SQLite,
        Redis,
        MongoDB,
    };
};

#endif // TCACHEFACTORY_H
