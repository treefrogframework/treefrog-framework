/* Copyright (c) 2013-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tepollsocket.h"
#include "tatomicptr.h"
#include "tepoll.h"
#include "tfcore.h"
#include "tsendbuffer.h"
#include <THttpHeader>
#include <TSystemGlobal>
#include <TWebApplication>
#include <TMultiplexingServer>
#include <QFileInfo>
#include <QSet>
#include <atomic>
#include <sys/types.h>

class SendData;

namespace {
int sendBufSize = 0;
int recvBufSize = 0;
std::atomic<int> socketCounter {0};
QSet<TEpollSocket *> socketManager;
}


TSendBuffer *TEpollSocket::createSendBuffer(const QByteArray &header, const QFileInfo &file, bool autoRemove, const TAccessLogger &logger)
{
    return new TSendBuffer(header, file, autoRemove, logger);
}


TSendBuffer *TEpollSocket::createSendBuffer(const QByteArray &data)
{
    return new TSendBuffer(data);
}


void TEpollSocket::initBuffer(int socketDescriptor)
{
    constexpr int BUF_SIZE = 128 * 1024;

    if (Q_UNLIKELY(sendBufSize == 0)) {
        // Creates a common buffer
        int res;
        socklen_t optlen;

        optlen = sizeof(int);
        res = getsockopt(socketDescriptor, SOL_SOCKET, SO_SNDBUF, &sendBufSize, &optlen);
        if (res < 0) {
            sendBufSize = BUF_SIZE;
        }
        optlen = sizeof(int);
        res = getsockopt(socketDescriptor, SOL_SOCKET, SO_RCVBUF, &recvBufSize, &optlen);
        if (res < 0) {
            recvBufSize = BUF_SIZE;
        }
    }
}


TEpollSocket::TEpollSocket(int socketDescriptor, const QHostAddress &address) :
    _sd(socketDescriptor),
    _clientAddr(address)
{
    tSystemDebug("TEpollSocket  socket:%d", _sd);
    socketManager.insert(this);
    socketCounter++;

    initBuffer(socketDescriptor);
}


TEpollSocket::~TEpollSocket()
{
    tSystemDebug("TEpollSocket::destructor");

    socketManager.remove(this);
    close();

    while (!_sendBuffer.isEmpty()) {
        TSendBuffer *buf = _sendBuffer.dequeue();
        delete buf;
    }

    socketCounter--;
}


/*!
  Receives data
  @return  0:success  -1:error
 */
int TEpollSocket::recv()
{
    int ret = 0;
    int err = 0;
    int len;

    for (;;) {
        void *buf = getRecvBuffer(recvBufSize);
        errno = 0;
        len = tf_recv(_sd, buf, recvBufSize, 0);
        err = errno;

        if (len <= 0) {
            break;
        }

        // Read successfully
        seekRecvBuffer(len);
    }

    if (!len && !err) {
        tSystemDebug("Socket disconnected : sd:%d", _sd);
        ret = -1;
    } else {
        if (len < 0 || err > 0) {
            switch (err) {
            case EAGAIN:
                break;

            case ECONNRESET:
                tSystemDebug("Socket disconnected : sd:%d  errno:%d", _sd, err);
                ret = -1;
                break;

            default:
                tSystemError("Failed recv : sd:%d  errno:%d  len:%d", _sd, err, len);
                ret = -1;
                break;
            }
        }
    }
    return ret;
}

/*!
  Sends data
  @return  0:success  -1:error
 */
int TEpollSocket::send()
{
    int ret = 0;

    if (_sendBuffer.isEmpty()) {
        pollOut = true;
        return ret;
    }
    pollOut = false;

    while (!_sendBuffer.isEmpty()) {
        TSendBuffer *buf = _sendBuffer.head();
        TAccessLogger &logger = buf->accessLogger();

        int len = 0;
        int err = 0;
        for (;;) {
            len = sendBufSize;
            void *data = buf->getData(len);
            if (len == 0) {
                break;
            }

            errno = 0;
            len = tf_send(_sd, data, len);
            err = errno;

            if (len <= 0) {
                break;
            }

            // Sent successfully
            buf->seekData(len);
            logger.setResponseBytes(logger.responseBytes() + len);
        }

        if (buf->atEnd()) {
            logger.write();  // Writes access log
            delete _sendBuffer.dequeue();  // delete send-buffer obj
        }

        if (len < 0) {
            switch (err) {
            case EAGAIN:
                break;

            case EPIPE:  // FALLTHRU
            case ECONNRESET:
                tSystemDebug("Socket disconnected : sd:%d  errno:%d", _sd, err);
                logger.setResponseBytes(-1);
                ret = -1;
                break;

            default:
                tSystemError("Failed send : sd:%d  errno:%d  len:%d", _sd, err, len);
                logger.setResponseBytes(-1);
                ret = -1;
                break;
            }

            break;
        }
    }
    return ret;
}


void *TEpollSocket::getRecvBuffer(int size)
{
    int len = _recvBuffer.size();
    _recvBuffer.reserve(len + size);
    return _recvBuffer.data() + len;
}


bool TEpollSocket::seekRecvBuffer(int pos)
{
    int size = _recvBuffer.size();
    if (Q_UNLIKELY(pos <= 0 || size + pos > _recvBuffer.capacity())) {
        Q_ASSERT(0);
        return false;
    }

    size += pos;
    _recvBuffer.resize(size);
    return true;
}


void TEpollSocket::enqueueSendData(TSendBuffer *buffer)
{
    _sendBuffer.enqueue(buffer);
}


void TEpollSocket::setSocketDescpriter(int socketDescriptor)
{
    _sd = socketDescriptor;
}


void TEpollSocket::close()
{
    if (_sd > 0) {
        tf_close_socket(_sd);
        _sd = 0;
    }
}


void TEpollSocket::sendData(const QByteArray &header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger)
{
    TEpoll::instance()->setSendData(this, header, body, autoRemove, accessLogger);
}


void TEpollSocket::sendData(const QByteArray &data)
{
    TEpoll::instance()->setSendData(this, data);
}


qint64 TEpollSocket::receiveData(char *buffer, qint64 length)
{
    qint64 len = std::min(length, _recvBuffer.length());
    if (len > 0) {
        memcpy(buffer, _recvBuffer.data(), len);
        _recvBuffer.remove(0, len);
    }
    return len;
}


QByteArray TEpollSocket::receiveAll()
{
    QByteArray res = _recvBuffer;
    _recvBuffer.clear();
    return res;
}


bool TEpollSocket::waitForDataSent(int msecs)
{
    int ms = msecs;
    QElapsedTimer elapsed;
    elapsed.start();

    while (!isDataSent()) {
        if (TMultiplexingServer::instance()->processEvents(ms) < 0) {
            break;
        }

        ms = msecs - elapsed.elapsed();
        if (ms <= 0) {
            break;
        }
    }
    return isDataSent();
}


bool TEpollSocket::waitForDataReceived(int msecs)
{
    int ms = msecs;
    QElapsedTimer elapsed;
    elapsed.start();

    while (!isDataReceived()) {
        if (TMultiplexingServer::instance()->processEvents(ms) < 0) {
            break;
        }

        ms = msecs - elapsed.elapsed();
        if (ms <= 0) {
            break;
        }
    }
    return isDataReceived();
}


void TEpollSocket::disconnect()
{
    TEpoll::instance()->setDisconnect(this);
}


void TEpollSocket::switchToWebSocket(const THttpRequestHeader &header)
{
    TEpoll::instance()->setSwitchToWebSocket(this, header);
}


int TEpollSocket::bufferedListCount() const
{
    return _sendBuffer.count();
}


QSet<TEpollSocket *> TEpollSocket::allSockets()
{
    return socketManager;
}
