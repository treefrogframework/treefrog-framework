#pragma once
#include <QByteArray>
#include <QMap>
#include <QVariant>
#include <TGlobal>


class T_CORE_EXPORT TCacheStore {
public:
    enum DbType {
        SQL,
        KVS,
        Invalid,
    };

    virtual ~TCacheStore() { }
    virtual QString key() const = 0;
    virtual DbType dbType() const = 0;
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual QByteArray get(const QByteArray &key) = 0;
    virtual bool set(const QByteArray &key, const QByteArray &value, int seconds) = 0;
    virtual bool remove(const QByteArray &key) = 0;
    virtual void clear() = 0;
    virtual void gc() = 0;
    virtual QMap<QString, QVariant> defaultSettings() const { return QMap<QString, QVariant>(); }
};

