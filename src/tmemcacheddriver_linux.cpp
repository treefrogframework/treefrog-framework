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


bool TMemcachedDriver::open(const QString &, const QString &, const QString &, const QString &host, quint16 port, const QString &)
{
    if (isOpen()) {
        return true;
    }

    _host = (host.isEmpty()) ? "localhost" : host;
    _port = (port == 0) ? DEFAULT_PORT : port;
    tSystemDebug("memcached open host:%s  port:%d", qUtf8Printable(_host), _port);

    _client = new TTcpSocket;
    _client->setSocketOption(IPPROTO_TCP, TCP_NODELAY, 1);
    _client->connectToHost(_host, _port);

    bool ret = _client->waitForConnected(1000);
    if (ret) {
        tSystemDebug("Memcached open successfully");
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
        tSystemError("Not open memcached session  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    qint64 len = _client->sendData(command);
    if (len < 0) {
        tSystemError("Socket send error  [%s:%d]", __FILE__, __LINE__);
        return false;
    }
    return _client->waitForDataSent(5000);
}


QByteArray TMemcachedDriver::readReply(int msecs)
{
    if (!isOpen()) {
        tSystemError("Not open memcached session  [%s:%d]", __FILE__, __LINE__);
        return QByteArray();
    }

    bool ret = _client->waitForDataReceived(msecs);
    if (!ret) {
        tSystemWarn("memcached response timeout");
        return QByteArray();
    }

    return _client->receiveAll();
}


void TMemcachedDriver::moveToThread(QThread *)
{
}
