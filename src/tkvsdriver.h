#pragma once
#include <QString>
#include <TGlobal>

class QThread;


class T_CORE_EXPORT TKvsDriver {
public:
    virtual ~TKvsDriver() { }
    virtual QString key() const = 0;
    virtual bool open(const QString &db, const QString &user, const QString &password, const QString &host, quint16 port, const QString &options) = 0;
    virtual void close() = 0;
    virtual bool command(const QString &) { return false; }
    virtual bool isOpen() const = 0;
    virtual void moveToThread(QThread *) { }
};

