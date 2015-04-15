#ifndef TSYSTEMBUS_H
#define TSYSTEMBUS_H

#include <QObject>
#include <TGlobal>
#include "tsystemglobal.h"

class QSocketNotifier;
class TSystemBusMessage;


class T_CORE_EXPORT TSystemBus : public QObject
{
    Q_OBJECT
public:
    bool send(const TSystemBusMessage &message) const;
    bool send(Tf::ServerOpCode opcode, const QString &dst, const QByteArray &payload) const;
    TSystemBusMessage recv();

    static TSystemBus *instance();
    static void instantiate();

public slots:
    void readStdIn();

signals:
    void readyRead();

private:
    QSocketNotifier *stdinNotifier;
    QByteArray buffer;

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
