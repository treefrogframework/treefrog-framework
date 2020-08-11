#pragma once
#include "tcachestore.h"
#include <TGlobal>


class T_CORE_EXPORT TCacheRedisStore : public TCacheStore {
public:
    virtual ~TCacheRedisStore() { }

    QString key() const override { return QLatin1String("redis"); }
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
    TCacheRedisStore();

    friend class TCacheFactory;
};

