/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tredisdriver.h"
#include "tsystemglobal.h"
#include <QTcpSocket>
#include <QThread>
#include <TApplicationServerBase>
using namespace Tf;

constexpr int DEFAULT_PORT = 6379;
constexpr int SEND_BUF_SIZE = 64 * 1024;
constexpr int RECV_BUF_SIZE = 64 * 1024;


TRedisDriver::~TRedisDriver()
{
    close();
    delete _client;
}


bool TRedisDriver::isOpen() const
{
    return (_client) ? (_client->state() == QAbstractSocket::ConnectedState) : false;
}


bool TRedisDriver::open(const QString &, const QString &, const QString &, const QString &host, uint16_t port, const QString &)
{
    if (isOpen()) {
        return true;
    }

    if (!_client) {
        _client = new QTcpSocket();
    }

    if (_client->state() != QAbstractSocket::UnconnectedState) {
        return false;
    }

    // Sets socket options
    _client->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // Sets buffer size of socket
    int val = _client->socketOption(QAbstractSocket::SendBufferSizeSocketOption).toInt();
    if (val < SEND_BUF_SIZE) {
        _client->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, SEND_BUF_SIZE);
    }

    val = _client->socketOption(QAbstractSocket::ReceiveBufferSizeSocketOption).toInt();
    if (val < RECV_BUF_SIZE) {
        _client->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, RECV_BUF_SIZE);
    }

    _host = (host.isEmpty()) ? "localhost" : host;
    _port = (port == 0) ? DEFAULT_PORT : port;

    tSystemDebug("Redis open host:{}  port:{}", _host, _port);
    _client->connectToHost(_host, _port);

    bool ret = _client->waitForConnected(1000);
    if (Q_LIKELY(ret)) {
        tSystemDebug("Redis open successfully");
    } else {
        tSystemError("Redis open failed");
        close();
    }
    return ret;
}


bool TRedisDriver::writeCommand(const QByteArray &command)
{
    return (_client) ? _client->write(command) : false;
}


void TRedisDriver::close()
{
    if (_client) {
        _client->close();
    }
}


bool TRedisDriver::readReply()
{
    if (Q_UNLIKELY(!isOpen())) {
        tSystemError("Not open Redis session  [{}:{}]", __FILE__, __LINE__);
        return false;
    }

    bool ret = _client->waitForReadyRead(5000);
    if (ret) {
        _buffer += _client->readAll();
    } else {
        tSystemWarn("Redis response timeout");
    }

    //tSystemDebug("#Redis response length: {}", _buffer.length());
    //tSystemDebug("#Redis response data: {}", _buffer.data());
    return ret;
}


void TRedisDriver::moveToThread(QThread *thread)
{
    int socket = 0;
    QAbstractSocket::SocketState state = QAbstractSocket::ConnectedState;

    if (_client) {
        socket = _client->socketDescriptor();

        if (socket > 0) {
            socket = TApplicationServerBase::duplicateSocket(socket);
            state = _client->state();
        }
        delete _client;
    }

    _client = new QTcpSocket;
    _client->moveToThread(thread);

    if (socket > 0) {
        _client->setSocketDescriptor(socket, state);
    }
}
