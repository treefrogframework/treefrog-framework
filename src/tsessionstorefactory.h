#pragma once
#include <QStringList>
#include <TGlobal>

class TSessionStore;


class T_CORE_EXPORT TSessionStoreFactory {
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
};

