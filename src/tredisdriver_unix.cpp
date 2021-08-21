/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tfcore_unix.h"
#include "tredisdriver.h"
#include "tsystemglobal.h"
#include <QTcpSocket>
#include <TApplicationServerBase>
using namespace Tf;

constexpr int DEFAULT_PORT = 6379;
constexpr int SEND_BUF_SIZE = 128 * 1024;
constexpr int RECV_BUF_SIZE = 128 * 1024;


TRedisDriver::TRedisDriver() :
    TKvsDriver()
{
    _buffer.reserve(1023);
}


TRedisDriver::~TRedisDriver()
{
    close();
}


bool TRedisDriver::isOpen() const
{
    return _socket > 0;
}


bool TRedisDriver::open(const QString &, const QString &, const QString &, const QString &host, quint16 port, const QString &)
{
    if (isOpen()) {
        return true;
    }

    QTcpSocket tcpSocket;

    // Sets socket options
    tcpSocket.setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // Sets buffer size of socket
    int val = tcpSocket.socketOption(QAbstractSocket::SendBufferSizeSocketOption).toInt();
    if (val < SEND_BUF_SIZE) {
        tcpSocket.setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, SEND_BUF_SIZE);
    }

    val = tcpSocket.socketOption(QAbstractSocket::ReceiveBufferSizeSocketOption).toInt();
    if (val < RECV_BUF_SIZE) {
        tcpSocket.setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, RECV_BUF_SIZE);
    }

    _host = (host.isEmpty()) ? "localhost" : host;
    _port = (port == 0) ? DEFAULT_PORT : port;

    tSystemDebug("Redis open host:%s  port:%d", qUtf8Printable(_host), _port);
    tcpSocket.connectToHost(_host, _port);

    bool ret = tcpSocket.waitForConnected(5000);
    if (Q_LIKELY(ret)) {
        tSystemDebug("Redis open successfully");
    } else {
        tSystemError("Redis open failed");
        close();
        return false;
    }

    _socket = TApplicationServerBase::duplicateSocket(tcpSocket.socketDescriptor());
    return _socket > 0;
}


void TRedisDriver::close()
{
    if (_socket > 0) {
        tf_close_socket(_socket);
        _socket = 0;
    }
}


bool TRedisDriver::writeCommand(const QByteArray &command)
{
    if (Q_UNLIKELY(!isOpen())) {
        tSystemError("Not open Redis session  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    qint64 total = 0;
    while (total < command.length()) {
        if (tf_poll_send(_socket, 5000) > 0) {
            qint64 len = tf_send(_socket, command.data() + total, command.length() - total);
            if (len < 0) {
                tSystemError("Socket send error  [%s:%d]", __FILE__, __LINE__);
                break;
            }
            total += len;
        } else {
            tSystemError("Socket poll error  [%s:%d]", __FILE__, __LINE__);
            break;
        }
    }
    return total == command.length();
}


bool TRedisDriver::readReply()
{
    if (Q_UNLIKELY(!isOpen())) {
        tSystemError("Not open Redis session  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    QByteArray buf;
    buf.reserve(RECV_BUF_SIZE);
    int timeout = 5000;
    int len = 0;

    while (tf_poll_recv(_socket, timeout) > 0) {
        len = tf_recv(_socket, buf.data(), RECV_BUF_SIZE, 0);
        if (len <= 0) {
            tSystemError("Socket recv error  [%s:%d]", __FILE__, __LINE__);
            break;
        }

        buf.resize(len);
        _buffer += buf;
        if (len < RECV_BUF_SIZE) {
            break;
        }
        timeout = 1;
    }

    return len > 0;
}


void TRedisDriver::moveToThread(QThread *)
{
}
