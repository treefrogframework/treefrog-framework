#pragma once
#include "tcachestore.h"
#include <TGlobal>


class T_CORE_EXPORT TCacheMemcachedStore : public TCacheStore {
public:
    virtual ~TCacheMemcachedStore() { }

    QString key() const override { return QLatin1String("memcached"); }
    DbType dbType() const override { return KVS; }
    bool open() override;
    void close() override;

    QByteArray get(const QByteArray &key) override;
    bool set(const QByteArray &key, const QByteArray &value, int seconds) override;
    bool remove(const QByteArray &key) override;
    void clear() override;
    void gc() override;
    QMap<QString, QVariant> defaultSettings() const override;

protected:
    TCacheMemcachedStore();

    friend class TCacheFactory;
};
