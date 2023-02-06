/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tfcore_unix.h"
#include "tredisdriver.h"
#include "ttcpsocket.h"
#include "tsystemglobal.h"
#include <TApplicationServerBase>
#include <netinet/tcp.h>

constexpr int DEFAULT_PORT = 6379;


TRedisDriver::~TRedisDriver()
{
    close();
    delete _client;
}


bool TRedisDriver::isOpen() const
{
    return (_client) ? _client->state() == Tf::SocketState::Connected : false;
}


bool TRedisDriver::open(const QString &, const QString &, const QString &, const QString &host, quint16 port, const QString &)
{
    if (isOpen()) {
        return true;
    }

    _host = (host.isEmpty()) ? "localhost" : host;
    _port = (port == 0) ? DEFAULT_PORT : port;
    tSystemDebug("Redis open host:%s  port:%d", qUtf8Printable(_host), _port);

    _client = new TTcpSocket;
    _client->setSocketOption(IPPROTO_TCP, TCP_NODELAY, 1);
    _client->connectToHost(_host, _port);

    bool ret = _client->waitForConnected(1000);
    if (ret) {
        tSystemDebug("Redis open successfully");
    } else {
        tSystemError("Redis open failed");
        close();
    }
    return ret;
}


void TRedisDriver::close()
{
    if (isOpen()) {
        _client->close();
    }
}


bool TRedisDriver::writeCommand(const QByteArray &command)
{
    bool ret = false;

    if (Q_UNLIKELY(!isOpen())) {
        tSystemError("Not open Redis session  [%s:%d]", __FILE__, __LINE__);
        return ret;
    }

    qint64 len = _client->sendData(command);
    if (len < 0) {
        tSystemError("Socket send error  [%s:%d]", __FILE__, __LINE__);
    } else {
        ret = _client->waitForDataSent(5000);
    }
    return ret;
}


bool TRedisDriver::readReply()
{
    if (!isOpen()) {
        tSystemError("Not open Redis session  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    if (!_client->waitForDataReceived(5000)) {
        tSystemWarn("Redis response timeout");
        return false;
    }

    qint64 recvlen = _client->receivedSize();
    if (recvlen <= 0) {
        tSystemError("Socket recv error  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    auto len = _buffer.length();
    _buffer.reserve(len + recvlen);
    _client->receiveData(_buffer.data() + len, recvlen);
    _buffer.resize(len + recvlen);
    // Don't use _client->receiveAll() here,
    // occur 'double free or corruption'..

    return true;
}


void TRedisDriver::moveToThread(QThread *)
{
}
