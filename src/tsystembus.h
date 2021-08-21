#pragma once
#include "tsystemglobal.h"
#include <QList>
#include <QLocalSocket>
#include <QMutex>
#include <QObject>
#include <TGlobal>

class TSystemBusMessage;


class T_CORE_EXPORT TSystemBus : public QObject {
    Q_OBJECT
public:
    ~TSystemBus();
    bool send(const TSystemBusMessage &message);
    bool send(Tf::SystemOpCode opcode, const QString &dst, const QByteArray &payload);
    TSystemBusMessage recv();
    QList<TSystemBusMessage> recvAll();
    void connect();

    static TSystemBus *instance();
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
    QLocalSocket *busSocket {nullptr};
    QByteArray readBuffer;
    QByteArray sendBuffer;
    QMutex mutexRead;
    QMutex mutexWrite;

    TSystemBus();
    T_DISABLE_COPY(TSystemBus)
    T_DISABLE_MOVE(TSystemBus)
};


class T_CORE_EXPORT TSystemBusMessage {
public:
    TSystemBusMessage();
    TSystemBusMessage(quint8 opcode, const QByteArray &data);
    TSystemBusMessage(quint8 opcode, const QString &target, const QByteArray &data);

    bool firstBit() const { return _firstByte & 0x80; }
    bool rsvBit() const { return _firstByte & 0x40; }
    Tf::SystemOpCode opCode() const { return (Tf::SystemOpCode)(_firstByte & 0x3F); }
    QString target() const;
    QByteArray data() const;

    int payloadLength() const { return _payload.length(); }
    QByteArray toByteArray() const;
    bool isValid() const { return _valid; }

    static TSystemBusMessage parse(QByteArray &bytes);

private:
    const QByteArray &payload() const { return _payload; }
    bool validate();

    quint8 _firstByte {0};
    QByteArray _payload;
    bool _valid {false};

    friend class TSystemBus;
};

