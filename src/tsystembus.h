#ifndef TSYSTEMBUS_H
#define TSYSTEMBUS_H

#include <QObject>
#include <QMutex>
#include <QLocalSocket>
#include <TGlobal>
#include "tsystemglobal.h"

class TSystemBusMessage;


class T_CORE_EXPORT TSystemBus : public QObject
{
    Q_OBJECT
public:
    ~TSystemBus();
    bool send(const TSystemBusMessage &message);
    bool send(Tf::ServerOpCode opcode, const QString &dst, const QByteArray &payload);
    TSystemBusMessage recv();
    void connect();

    static TSystemBus *instance();
    static void instantiate();
    static QString connectionName();
    static QString connectionName(qint64 pid);

signals:
    void readyReceive();
    void disconnected();

protected slots:
    void readBus();
    void writeBus();
    void handleError(QLocalSocket::LocalSocketError error);

private:
    QLocalSocket *busSocket;
    QByteArray readBuffer;
    QByteArray writeBuffer;
    QMutex mutexRead;
    QMutex mutexWrite;

    TSystemBus();
    Q_DISABLE_COPY(TSystemBus)
};


class T_CORE_EXPORT TSystemBusMessage
{
public:
    int opCode;
    QString dst;
    QByteArray payload;

    TSystemBusMessage(int opcode, const QString &dst, const QByteArray &payload);
    bool validate();
};

#endif // TSYSTEMBUS_H
