/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMutex>
#include <QDataStream>
#include <QSocketNotifier>
#include <TApplicationServerBase>
#include "tsystembus.h"
#include "tsystemglobal.h"
#include "tfcore.h"

static TSystemBus *systemBus = nullptr;
const int HEADER_LEN = 5;

// static bool select(int fd, int timeout, bool checkRead, bool checkWrite)
// {
// #if Q_OS_WIN
//     // doesn't work for descriptors of stdin and stdout on Windows
//     Q_UNUSED(fd);
//     Q_UNUSED(timeout);
//     Q_UNUSED(checkRead);
//     Q_UNUSED(checkWrite);
//     return true;
// #else
//     fd_set fdread;
//     FD_ZERO(&fdread);
//     if (checkRead) {
//         FD_SET(fd, &fdread);
//     }

//     fd_set fdwrite;
//     FD_ZERO(&fdwrite);
//     if (checkWrite) {
//         FD_SET(fd, &fdwrite);
//     }

//     struct timeval tv;
//     tv.tv_sec = timeout / 1000;
//     tv.tv_usec = (timeout % 1000) * 1000;

//     int ret;
//     ret = tf_select(fd + 1, &fdread, &fdwrite, nullptr, (timeout < 0 ? nullptr : &tv));
//     return ret > 0;
// #endif
// }


void TSystemBus::instantiate()
{
    if (!systemBus) {
        systemBus = new TSystemBus;
    }
}


TSystemBus::TSystemBus()
    : readFd(0), writeFd(0), readNotifier(nullptr), writeNotifier(nullptr), readBuffer(), writeBuffer()
{
    readFd = tf_dup(tf_fileno(stdin));
    writeFd = tf_dup(tf_fileno(stdout));

#ifndef Q_OS_WIN
    ::fcntl(readFd, F_SETFL, ::fcntl(readFd, F_GETFL) | O_NONBLOCK);   // non-block
    ::fcntl(writeFd, F_SETFL, ::fcntl(writeFd, F_GETFL) | O_NONBLOCK); // non-block
#endif

    readNotifier = new QSocketNotifier(readFd, QSocketNotifier::Read, this);
    writeNotifier = new QSocketNotifier(writeFd, QSocketNotifier::Write, this);
    connect(readNotifier, SIGNAL(activated(int)), this, SLOT(readStdIn()));
    connect(writeNotifier, SIGNAL(activated(int)), this, SLOT(writeStdOut()));
    readNotifier->setEnabled(true);
    writeNotifier->setEnabled(false);
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
#if 0
    QMutexLocker locker(&mutexWrite);
    int total = 0;
    int len;
    for (;;) {
        len = tf_write(writeFd, buf.data() + total, buf.length() - total);
        if (len < 0) {
            tSystemError("System Bus write error  [%s:%d]", __FILE__, __LINE__);
            break;
        }

        total += len;
        if (total == buf.length()) {
            break;
        }

        if (!select(writeFd, 1000, false, true)) {
            tSystemError("System Bus write-wait error  [%s:%d]", __FILE__, __LINE__);
            break;
        }
    }
    return len > 0;
#else
    QMutexLocker locker(&mutexWrite);
    writeBuffer += buf;
    writeNotifier->setEnabled(true);
    return true;
#endif
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


void TSystemBus::readStdIn()
{
    const int BUFLEN = 4096;
    QMutexLocker locker(&mutexRead);

    readNotifier->setEnabled(false);
    int currentLen = readBuffer.length();
    readBuffer.reserve(currentLen + BUFLEN);

    int len = tf_read(readFd, readBuffer.data() + currentLen, BUFLEN);
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
                emit readyRead();
            } else {
                tSystemError("Invalid opcode: %d  [%s:%d]", opcode, __FILE__, __LINE__);
                readBuffer.remove(0, length + HEADER_LEN);
            }
        }
    }

    readNotifier->setEnabled(true);
}


void TSystemBus::writeStdOut()
{
    writeNotifier->setEnabled(false);
    QMutexLocker locker(&mutexWrite);

    if (Q_LIKELY(!writeBuffer.isEmpty())) {
        int len = tf_write(writeFd, writeBuffer.data(), writeBuffer.length());
        if (Q_UNLIKELY(len <= 0)) {
            tSystemError("System Bus write error  res:%d  [%s:%d]", len, __FILE__, __LINE__);
            writeBuffer.clear();
        } else {
            if (len == writeBuffer.length()) {
                writeBuffer.truncate(0);
            } else {
                writeBuffer.remove(0, len);
            }
        }
    }

    writeNotifier->setEnabled(!writeBuffer.isEmpty());
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
