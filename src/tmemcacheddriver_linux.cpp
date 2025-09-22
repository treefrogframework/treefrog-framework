/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tfcore_unix.h"
#include "tmemcacheddriver.h"
#include "ttcpsocket.h"
#include "tsystemglobal.h"
#include <TApplicationServerBase>
#include <netinet/tcp.h>


TMemcachedDriver::~TMemcachedDriver()
{
    close();
    delete _client;
}


bool TMemcachedDriver::isOpen() const
{
    return (_client) ? _client->state() == Tf::SocketState::Connected : false;
}


bool TMemcachedDriver::open(const QString &, const QString &, const QString &, const QString &host, uint16_t port, const QString &)
{
    if (isOpen()) {
        return true;
    }

    _host = (host.isEmpty()) ? "localhost" : host;
    _port = (port == 0) ? DEFAULT_PORT : port;
    tSystemDebug("memcached open host:{}  port:{}", _host, _port);

    _client = new TTcpSocket;
    _client->setSocketOption(IPPROTO_TCP, TCP_NODELAY, 1);
    _client->connectToHost(_host, _port);

    bool ret = _client->waitForConnected(1000);
    if (ret) {
        tSystemDebug("Memcached open successfully. sd:{}", _client->socketDescriptor());
    } else {
        tSystemError("Memcached open failed");
        close();
    }
    return ret;
}


void TMemcachedDriver::close()
{
    if (isOpen()) {
        _client->close();
    }
}


bool TMemcachedDriver::writeCommand(const QByteArray &command)
{
    if (!isOpen()) {
        tSystemError("Not open memcached session  [{}:{}]", __FILE__, __LINE__);
        return false;
    }

    int64_t len = _client->sendData(command);
    if (len < 0) {
        tSystemError("Socket send error  [{}:{}]", __FILE__, __LINE__);
        return false;
    }
    return _client->waitForDataSent(5000);
}


QByteArray TMemcachedDriver::readReply(int msecs)
{
    QByteArray buffer;

    if (!isOpen()) {
        tSystemError("Not open memcached session  [{}:{}]", __FILE__, __LINE__);
        return buffer;
    }

    if (!_client->waitForDataReceived(msecs)) {
        tSystemWarn("memcached response timeout");
        return buffer;
    }

    int64_t recvlen = _client->receivedSize();
    if (recvlen <= 0) {
        tSystemError("Socket recv error  [{}:{}]", __FILE__, __LINE__);
        return buffer;
    }

    buffer.reserve(recvlen);
    _client->receiveData(buffer.data(), recvlen);
    buffer.resize(recvlen);
    // Don't use _client->receiveAll() here,
    // occur 'double free or corruption'..
    return buffer;
}


void TMemcachedDriver::moveToThread(QThread *)
{
}
