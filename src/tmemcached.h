#pragma once
#include <QByteArray>
#include <QStringList>
#include <TGlobal>
#include <TKvsDatabase>
#include <TfNamespace>

class TMemcachedDriver;


class T_CORE_EXPORT TMemcached {
public:
    TMemcached();
    TMemcached(const TMemcached &other);
    virtual ~TMemcached() { }

    bool isOpen() const;
    //bool exists(const QByteArray &key);

private:
    TMemcached(Tf::KvsEngine engine);
    TMemcachedDriver *driver();
    const TMemcachedDriver *driver() const;

    TKvsDatabase database;

    friend class TCacheRedisStore;
};
