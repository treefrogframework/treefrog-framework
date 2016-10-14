#ifndef TSYSTEMBUS_H
#define TSYSTEMBUS_H

#include <QObject>
#include <QList>
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
    QList<TSystemBusMessage> recvAll();
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
    QByteArray sendBuffer;
    QMutex mutexRead;
    QMutex mutexWrite;

    TSystemBus();
    T_DISABLE_COPY(TSystemBus)
    T_DISABLE_MOVE(TSystemBus)
};


class T_CORE_EXPORT TSystemBusMessage
{
public:
    enum OpCode {
        Invalid                 = 0x00,
        WebSocketSendText       = 0x01,
        WebSocketSendBinary     = 0x02,
        WebSocketPublishText    = 0x03,
        WebSocketPublishBinary  = 0x04,
        MaxOpCode               = 0x04,
    };

    TSystemBusMessage();
    TSystemBusMessage(quint8 opcode, const QByteArray &data);
    TSystemBusMessage(quint8 opcode, const QString &target, const QByteArray &data);

    bool firstBit() const { return firstByte_ & 0x80; }
    bool rsvBit() const { return firstByte_ & 0x40; }
    OpCode opCode() const { return (OpCode)(firstByte_ & 0x3F); }
    QString target() const;
    QByteArray data() const;

    int payloadLength() const { return payload_.length(); }
    QByteArray toByteArray() const;
    bool isValid() const { return valid_; }

    static TSystemBusMessage parse(QByteArray &bytes);

private:
    const QByteArray &payload() const { return payload_; }
    bool validate();

    quint8 firstByte_;
    QByteArray payload_;
    bool valid_;

    friend class TSystemBus;
};

#endif // TSYSTEMBUS_H
