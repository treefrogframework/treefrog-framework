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

constexpr int DEFAULT_PORT = 11211;


TMemcachedDriver::TMemcachedDriver() :
    TKvsDriver()
{
    _buffer.reserve(1023);
}


TMemcachedDriver::~TMemcachedDriver()
{
    close();
    delete _client;
}


bool TMemcachedDriver::isOpen() const
{
    return (_client) ? _client->state() == Tf::SocketState::Connected : false;
}


bool TMemcachedDriver::open(const QString &, const QString &, const QString &, const QString &host, quint16 port, const QString &)
{
    if (isOpen()) {
        return true;
    }

    _host = (host.isEmpty()) ? "localhost" : host;
    _port = (port == 0) ? DEFAULT_PORT : port;
    tSystemDebug("memcached open host:%s  port:%d", qUtf8Printable(_host), _port);

    _client = new TTcpSocket;
    _client->setSocketOption(SOL_TCP, TCP_NODELAY, 1);
    _client->connectToHost(_host, _port);
    return _client->waitForConnected(5000);
}


void TMemcachedDriver::close()
{
    if (isOpen()) {
        _client->close();
    }
}


bool TMemcachedDriver::writeCommand(const QByteArray &command)
{
    bool ret = false;

    if (Q_UNLIKELY(!isOpen())) {
        tSystemError("Not open memcached session  [%s:%d]", __FILE__, __LINE__);
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


bool TMemcachedDriver::readReply()
{
    if (Q_UNLIKELY(!isOpen())) {
        tSystemError("Not open memcached session  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    QByteArray buf;
    if (_client->waitForDataReceived(5000)) {
        buf = _client->receiveAll();
        if (buf.isEmpty()) {
            tSystemError("Socket recv error  [%s:%d]", __FILE__, __LINE__);
        } else {
            _buffer += buf;
        }
    }
    return !buf.isEmpty();
}


void TMemcachedDriver::moveToThread(QThread *)
{
}
