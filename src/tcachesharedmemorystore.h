#pragma once
#include "tcachestore.h"
#include <TGlobal>

class TSharedMemoryKvs;


class T_CORE_EXPORT TCacheSharedMemoryStore : public TCacheStore {
public:
    ~TCacheSharedMemoryStore();
    QString key() const override { return QLatin1String("memory"); }
    DbType dbType() const override { return KVS; }
    void init() override;
    void cleanup() override;
    bool open() override { return true; }
    void close() override {}
    QByteArray get(const QByteArray &key) override;
    bool set(const QByteArray &key, const QByteArray &value, int seconds) override;
    bool remove(const QByteArray &key) override;
    void clear() override;
    void gc() override;
    QMap<QString, QVariant> defaultSettings() const override;

private:
    TCacheSharedMemoryStore();

    friend class TCacheFactory;
    T_DISABLE_COPY(TCacheSharedMemoryStore)
    T_DISABLE_MOVE(TCacheSharedMemoryStore)
};
