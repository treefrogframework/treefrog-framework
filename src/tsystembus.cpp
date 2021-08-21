/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsystembus.h"
#include "tfcore.h"
#include "tprocessinfo.h"
#include "tsystemglobal.h"
#include <QDataStream>
#include <QLocalSocket>
#include <QMutex>
#include <QStringList>
#include <TApplicationServerBase>
#include <TWebApplication>

constexpr int HEADER_LEN = 5;
constexpr auto SYSTEMBUS_DOMAIN_PREFIX = "treefrog_systembus_";


TSystemBus *TSystemBus::instance()
{
    static TSystemBus *systemBus = []() {
        auto *bus = new TSystemBus();
        bus->connect();
        return bus;
    }();
    return systemBus;
}


TSystemBus::TSystemBus() :
    busSocket(new QLocalSocket)
{
    QObject::connect(busSocket, SIGNAL(readyRead()), this, SLOT(readBus()));
    QObject::connect(busSocket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
    QObject::connect(busSocket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(handleError(QLocalSocket::LocalSocketError)));
}


TSystemBus::~TSystemBus()
{
    busSocket->close();
    delete busSocket;
}


bool TSystemBus::send(const TSystemBusMessage &message)
{
    QMutexLocker locker(&mutexWrite);
    sendBuffer += message.toByteArray();
    QMetaObject::invokeMethod(this, "writeBus", Qt::QueuedConnection);  // Writes in main thread
    return true;
}


bool TSystemBus::send(Tf::SystemOpCode opcode, const QString &dst, const QByteArray &payload)
{
    return send(TSystemBusMessage(opcode, dst, payload));
}


QList<TSystemBusMessage> TSystemBus::recvAll()
{
    QList<TSystemBusMessage> ret;
    quint8 opcode;
    quint32 length;
    QMutexLocker locker(&mutexRead);

    for (;;) {
        QDataStream ds(readBuffer);
        ds.setByteOrder(QDataStream::BigEndian);
        ds >> opcode >> length;

        if ((uint)readBuffer.length() < length + HEADER_LEN) {
            break;
        }

        auto message = TSystemBusMessage::parse(readBuffer);
        if (message.isValid()) {
            ret << message;
        }
    }
    return ret;
}


void TSystemBus::readBus()
{
    bool ready = false;
    {
        QMutexLocker locker(&mutexRead);
        readBuffer += busSocket->readAll();

        QDataStream ds(readBuffer);
        ds.setByteOrder(QDataStream::BigEndian);
        quint8 opcode;
        quint32 length;
        ds >> opcode >> length;

        ready = ((uint)readBuffer.length() >= length + HEADER_LEN);
    }

    if (ready) {
        emit readyReceive();
    }
}


void TSystemBus::writeBus()
{
    QMutexLocker locker(&mutexWrite);
    tSystemDebug("TSystemBus::writeBus  len:%lld", sendBuffer.length());

    for (;;) {
        int len = busSocket->write(sendBuffer.data(), sendBuffer.length());

        if (Q_UNLIKELY(len < 0)) {
            tSystemError("System Bus write error  res:%d  [%s:%d]", len, __FILE__, __LINE__);
            sendBuffer.resize(0);
        } else {
            if (len > 0) {
                sendBuffer.remove(0, len);
            }
        }

        if (sendBuffer.isEmpty()) {
            break;
        }

        if (!busSocket->waitForBytesWritten(1000)) {
            tSystemError("System Bus write-wait error  res:%d  [%s:%d]", len, __FILE__, __LINE__);
            sendBuffer.resize(0);
            break;
        }
    }
}


void TSystemBus::connect()
{
    busSocket->connectToServer(connectionName());
}


void TSystemBus::handleError(QLocalSocket::LocalSocketError error)
{
    switch (error) {
    case QLocalSocket::PeerClosedError:
        tSystemError("Remote socket closed the connection");
        break;

    default:
        tSystemError("Local socket error : %d", (int)error);
        break;
    }
}


QString TSystemBus::connectionName()
{
#if defined(Q_OS_WIN) && !defined(TF_NO_DEBUG)
    constexpr auto PROCESS_NAME = "tadpoled";
#else
    constexpr auto PROCESS_NAME = "tadpole";
#endif

    qint64 pid = 0;
    QString cmd = TWebApplication::arguments().first();
    if (cmd.endsWith(QLatin1String(PROCESS_NAME))) {
        pid = TProcessInfo(TWebApplication::applicationPid()).ppid();
    } else {
        pid = TWebApplication::applicationPid();
    }

    return connectionName(pid);
}


QString TSystemBus::connectionName(qint64 pid)
{
    return SYSTEMBUS_DOMAIN_PREFIX + QString::number(pid);
}


TSystemBusMessage::TSystemBusMessage()
{
}


TSystemBusMessage::TSystemBusMessage(quint8 op, const QByteArray &d)
{
    _firstByte = 0x80 | (op & 0x3F);
    QDataStream ds(&_payload, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << QByteArray() << d;
}


TSystemBusMessage::TSystemBusMessage(quint8 op, const QString &t, const QByteArray &d)
{
    _firstByte = 0x80 | (op & 0x3F);
    QDataStream ds(&_payload, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << t << d;
}


QString TSystemBusMessage::target() const
{
    QString ret;
    QDataStream ds(_payload);
    ds.setByteOrder(QDataStream::BigEndian);
    ds >> ret;
    return ret;
}


QByteArray TSystemBusMessage::data() const
{
    QByteArray ret;
    QString target;
    QDataStream ds(_payload);
    ds.setByteOrder(QDataStream::BigEndian);
    ds >> target >> ret;
    return ret;
}


bool TSystemBusMessage::validate()
{
    _valid = true;
    _valid &= (firstBit() == true);
    _valid &= (rsvBit() == false);
    if (!_valid) {
        tSystemError("Invalid byte: 0x%x  [%s:%d]", _firstByte, __FILE__, __LINE__);
    }

    _valid &= (opCode() > 0 && opCode() <= Tf::MaxOpCode);
    if (!_valid) {
        tSystemError("Invalid opcode: %d  [%s:%d]", (int)opCode(), __FILE__, __LINE__);
    }
    return _valid;
}


QByteArray TSystemBusMessage::toByteArray() const
{
    QByteArray buf;
    buf.reserve(HEADER_LEN + _payload.length());

    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << _firstByte << (quint32)_payload.length();
    ds.writeRawData(_payload.data(), _payload.length());
    return buf;
}


TSystemBusMessage TSystemBusMessage::parse(QByteArray &bytes)
{
    QDataStream ds(bytes);
    ds.setByteOrder(QDataStream::BigEndian);

    quint8 opcode;
    quint32 length;
    ds >> opcode >> length;

    if ((uint)bytes.length() < HEADER_LEN || (uint)bytes.length() < HEADER_LEN + length) {
        tSystemError("Invalid length: %d  [%s:%d]", length, __FILE__, __LINE__);
        bytes.resize(0);
        return TSystemBusMessage();
    }

    TSystemBusMessage message;
    message._firstByte = opcode;
    message._payload = bytes.mid(HEADER_LEN, length);
    message.validate();
    bytes.remove(0, HEADER_LEN + length);
    return message;
}


/* Data format for system bus
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-----------+-----------------------------------------------+
 |F|R|  opcode   |              Payload length                   |
 |L|S|   (6)     |                  (32)                         |
 |G|V|           |                                               |
 +-+-+-----------+-----------------------------------------------+
 |               | Payload data : target (QString format)        |
 |               |          (x)                                  |
 +---------------+-----------------------------------------------+
 |            Payload data : QByteArray format                   |
 |                     (y)                                       |
 +---------------+-----------------------------------------------+
*/
