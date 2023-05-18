#pragma once
#include <QString>
#include <QByteArray>
#include <TGlobal>

class QThread;


class T_CORE_EXPORT TKvsDriver {
public:
    virtual ~TKvsDriver() { }
    virtual QString key() const = 0;
    virtual bool open(const QString &db, const QString &user, const QString &password, const QString &host, uint16_t port, const QString &options) = 0;
    virtual void close() = 0;
    virtual bool command(const QByteArray &) { return false; }
    virtual bool isOpen() const = 0;
    virtual void moveToThread(QThread *) { }
};
