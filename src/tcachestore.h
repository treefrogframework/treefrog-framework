#ifndef TCACHESTORE_H
#define TCACHESTORE_H

#include <TGlobal>
#include <QByteArray>


class T_CORE_EXPORT TCacheStore
{
public:
    virtual ~TCacheStore() {}
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    virtual int count() const = 0;
    virtual bool exists(const QByteArray &name) const = 0;
    virtual bool read(const QByteArray &name, QByteArray &blob, qint64 &timestamp) = 0;
    virtual bool write(const QByteArray &name, const QByteArray &blob, qint64 timestamp) = 0;
    virtual int remove(const QByteArray &name) = 0;
    virtual int removeAll() = 0;
    virtual void gc() = 0;
};

#endif // TCACHESTORE_H
