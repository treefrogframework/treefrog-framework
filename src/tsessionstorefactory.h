#ifndef TSESSIONSTOREFACTORY_H
#define TSESSIONSTOREFACTORY_H

#include <QStringList>
#include <TGlobal>

class TSessionStore;


class T_CORE_EXPORT TSessionStoreFactory
{
public:
    static QStringList keys();
    static TSessionStore *create(const QString &key);
    static void destroy(const QString &key, TSessionStore *store);

private:
    enum StoreType {
        Invalid = 0,
        SqlObject,
        Cookie,
        File,
        Plugin,
    };

    static void loadPlugins();
};

#endif // TSESSIONSTOREFACTORY_H
