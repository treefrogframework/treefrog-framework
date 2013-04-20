#ifndef TKVSDRIVER_H
#define TKVSDRIVER_H

#include <QString>
#include <TGlobal>


class T_CORE_EXPORT TKvsDriver
{
public:
    virtual ~TKvsDriver() { }
    virtual bool open(const QString &host, quint16 port) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
};

#endif // TKVSDRIVER_H
