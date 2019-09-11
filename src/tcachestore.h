#ifndef TCACHESTORE_H
#define TCACHESTORE_H

#include <TGlobal>
#include <QMap>
#include <QByteArray>
#include <QVariant>


class T_CORE_EXPORT TCacheStore
{
public:
    virtual ~TCacheStore() {}
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual QByteArray get(const QByteArray &key) = 0;
    virtual bool set(const QByteArray &key, const QByteArray &value, qint64 msecs) = 0;
    virtual bool remove(const QByteArray &key) = 0;
    virtual void clear() = 0;
    virtual void gc() = 0;
    virtual QMap<QString, QVariant> defaultSettings() const { return QMap<QString, QVariant>(); }
};

#endif // TCACHESTORE_H
