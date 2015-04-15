/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMutex>
#include <QDataStream>
#include <QFile>
#include <QSocketNotifier>
#include <TApplicationServerBase>
#include "tsystembus.h"
#include "tsystemglobal.h"

static TSystemBus *systemBus = nullptr;


void TSystemBus::instantiate()
{
    if (!systemBus) {
        systemBus = new TSystemBus;
    }
}


TSystemBus::TSystemBus()
    : buffer()
{
    stdinNotifier = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read, this);
    connect(stdinNotifier, SIGNAL(activated(int)), this, SLOT(readStdIn()));
    stdinNotifier->setEnabled(true);
}


bool TSystemBus::send(const TSystemBusMessage &message) const
{
    return send((Tf::ServerOpCode)message.opCode, message.dst, message.payload);
}


bool TSystemBus::send(Tf::ServerOpCode opcode, const QString &dst, const QByteArray &payload) const
{
    static QMutex mutex;

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
        ds << (int)buf.length() - 5;  // overwrite the length
    }

    //tSystemDebug("0x%x 0x%x 0x%x 0x%x 0x%x", (char)buf[0], (char)buf[1], (char)buf[2], (char)buf[3], (char)buf[4]);

    QFile stdOut;
    QMutexLocker locker(&mutex);
    stdOut.open(stdout, QIODevice::WriteOnly);

    int total = 0;
    int len;
    for (;;) {
        len = stdOut.write(buf.data() + total, buf.length() - total);
        if (len < 0) {
            tSystemError("System Bus write error  [%s:%d]", __FILE__, __LINE__);
            break;
        }

        total += len;
        if (total == buf.length()) {
            break;
        }
    }
    return len > 0;
}


TSystemBusMessage TSystemBus::recv()
{
    QDataStream ds(buffer);
    ds.setByteOrder(QDataStream::BigEndian);
    quint8 opcode;
    int length;
    QString dst;
    QByteArray payload;

    ds >> opcode >> length >> dst >> payload;
    TSystemBusMessage message(opcode, dst, payload);
    buffer.remove(0, length + 5);
    return message;
}


void TSystemBus::readStdIn()
{
    stdinNotifier->setEnabled(false);

    int currentLen = buffer.length();
    buffer.reserve(currentLen + 1024);

    int len = ::read(fileno(stdin), buffer.data() + currentLen, 1024);
    if (len <= 0) {
        tSystemError("stdin read error  [%s:%d]", __FILE__, __LINE__);
        buffer.clear();
    } else {
        buffer.resize(currentLen + len);

        QDataStream ds(buffer);
        ds.setByteOrder(QDataStream::BigEndian);
        quint8 opcode;
        int length;
        ds >> opcode >> length;
        if (buffer.length() >= length + 5) {
            if (opcode > 0 && opcode < (int)Tf::MaxServerOpCode) {
                emit readyRead();
            } else {
                tSystemError("Invalid opcode: %d  [%s:%d]", opcode, __FILE__, __LINE__);
                buffer.remove(0, length + 5);
            }
        }
    }

    stdinNotifier->setEnabled(true);
}


TSystemBus *TSystemBus::instance()
{
    return systemBus;
}


TSystemBusMessage::TSystemBusMessage(int o, const QString &d, const QByteArray &p)
    : opCode(o), dst(d), payload(p)
{ }


bool TSystemBusMessage::validate()
{
    if (opCode <= 0 || opCode >= (int)Tf::MaxServerOpCode)
        return false;

    return true;
}
