/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMutex>
#include <QDataStream>
#include <QLocalSocket>
#include <QTimer>
#include <TWebApplication>
#include <TApplicationServerBase>
#include "tsystembus.h"
#include "tsystemglobal.h"
#include "tprocessinfo.h"
#include "tfcore.h"

static TSystemBus *systemBus = nullptr;
const int HEADER_LEN = 5;
const QString SYSTEMBUS_DOMAIN_PREFIX = "treefrog_systembus_";


TSystemBus::TSystemBus()
    : readBuffer(), writeBuffer(), mutexRead(QMutex::Recursive), mutexWrite(QMutex::NonRecursive)
{
    busSocket = new QLocalSocket();
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
    return send((Tf::ServerOpCode)message.opCode, message.dst, message.payload);
}


bool TSystemBus::send(Tf::ServerOpCode opcode, const QString &dst, const QByteArray &payload)
{
    QByteArray buf;
    {
        QDataStream ds(&buf, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::BigEndian);
        ds << (quint8)opcode << (int)0 << dst << payload;
    }
    {
        QDataStream ds(&buf, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::BigEndian);
        ds.skipRawData(sizeof(quint8));
        ds << (int)buf.length() - HEADER_LEN;  // overwrite the length
    }
    //tSystemDebug("0x%x 0x%x 0x%x 0x%x 0x%x", (char)buf[0], (char)buf[1], (char)buf[2], (char)buf[3], (char)buf[4]);

    QMutexLocker locker(&mutexWrite);
    writeBuffer += buf;
    QTimer::singleShot(0, this, SLOT(writeBus()));   // Writes in main thread
    return true;
}


TSystemBusMessage TSystemBus::recv()
{
    QMutexLocker locker(&mutexRead);

    QDataStream ds(readBuffer);
    ds.setByteOrder(QDataStream::BigEndian);
    quint8 opcode;
    int length;
    QString dst;
    QByteArray payload;

    ds >> opcode >> length >> dst >> payload;
    TSystemBusMessage message(opcode, dst, payload);
    readBuffer.remove(0, length + HEADER_LEN);
    return message;
}


void TSystemBus::readBus()
{
    const int BUFLEN = 4096;
    QMutexLocker locker(&mutexRead);

    int currentLen = readBuffer.length();
    readBuffer.reserve(currentLen + BUFLEN);

    int len = busSocket->read(readBuffer.data() + currentLen, BUFLEN);
    if (Q_UNLIKELY(len <= 0)) {
        tSystemError("stdin read error  [%s:%d]", __FILE__, __LINE__);
        readBuffer.clear();
    } else {
        readBuffer.resize(currentLen + len);

        QDataStream ds(readBuffer);
        ds.setByteOrder(QDataStream::BigEndian);
        quint8 opcode;
        int length;
        ds >> opcode >> length;

        if (readBuffer.length() >= length + HEADER_LEN) {
            if (opcode > 0 && opcode < (int)Tf::MaxServerOpCode) {
                emit readyReceive();
            } else {
                tSystemError("Invalid opcode: %d  [%s:%d]", opcode, __FILE__, __LINE__);
                readBuffer.remove(0, length + HEADER_LEN);
            }
        }
    }

}


void TSystemBus::writeBus()
{
    QMutexLocker locker(&mutexWrite);

    while (!writeBuffer.isEmpty()) {
        int len = busSocket->write(writeBuffer.data(), writeBuffer.length());
        if (Q_UNLIKELY(len <= 0)) {
            tSystemError("System Bus write error  res:%d  [%s:%d]", len, __FILE__, __LINE__);
            writeBuffer.clear();
        } else {
            if (len == writeBuffer.length()) {
                writeBuffer.truncate(0);
                break;
            } else {
                writeBuffer.remove(0, len);
            }

            if (!busSocket->waitForBytesWritten(1000)) {
                tSystemError("System Bus write-wait error  res:%d  [%s:%d]", len, __FILE__, __LINE__);
                writeBuffer.clear();
                break;
            }
        }
    }
}


void TSystemBus::connect()
{
    busSocket->connectToServer(connectionName());
}


void TSystemBus::handleError(QLocalSocket::LocalSocketError error)
{
    tSystemError("Local socket error : %d", (int)error);
}



TSystemBus *TSystemBus::instance()
{
    return systemBus;
}


void TSystemBus::instantiate()
{
    if (!systemBus) {
        systemBus = new TSystemBus;
        systemBus->connect();
    }
}


QString TSystemBus::connectionName()
{
#if defined(Q_OS_WIN) && !defined(TF_NO_DEBUG)
    const QString processName = "tadpoled";
#else
    const QString processName = "tadpole";
#endif

    qint64 pid = 0;
    QString cmd = TWebApplication::arguments().first();
    if (cmd.lastIndexOf(processName) > 0) {
        pid = TProcessInfo(TWebApplication::applicationPid()).ppid();
    } else {
        pid = TProcessInfo(TWebApplication::applicationPid()).pid();
    }

    return connectionName(pid);
}


QString TSystemBus::connectionName(qint64 pid)
{
    return SYSTEMBUS_DOMAIN_PREFIX + QString::number(pid);
}


TSystemBusMessage::TSystemBusMessage(int o, const QString &d, const QByteArray &p)
    : opCode(o), dst(d), payload(p)
{ }


bool TSystemBusMessage::validate()
{
    if (opCode <= 0 || opCode >= (int)Tf::MaxServerOpCode) {
        return false;
    }
    return true;
}
