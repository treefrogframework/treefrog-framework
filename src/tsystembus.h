#ifndef TSYSTEMBUS_H
#define TSYSTEMBUS_H

#include <QObject>
#include <QMutex>
#include <TGlobal>
#include "tsystemglobal.h"

class QSocketNotifier;
class TSystemBusMessage;


class T_CORE_EXPORT TSystemBus : public QObject
{
    Q_OBJECT
public:
    bool send(const TSystemBusMessage &message);
    bool send(Tf::ServerOpCode opcode, const QString &dst, const QByteArray &payload);
    TSystemBusMessage recv();

    static TSystemBus *instance();
    static void instantiate();

public slots:
    void readStdIn();
    void writeStdOut();

signals:
    void readyRead();

private:
    int readFd;
    int writeFd;
    QSocketNotifier *readNotifier;
    QSocketNotifier *writeNotifier;
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
