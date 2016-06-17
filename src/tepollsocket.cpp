/* Copyright (c) 2013-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <sys/types.h>
#include <QUuid>
#include <QFileInfo>
#include <TWebApplication>
#include <TSystemGlobal>
#include <THttpHeader>
#include "tepollsocket.h"
#include "tepollhttpsocket.h"
#include "tepoll.h"
#include "tsendbuffer.h"
#include "tfcore.h"

class SendData;

static int sendBufSize = 0;
static int recvBufSize = 0;


TEpollSocket *TEpollSocket::accept(int listeningSocket)
{
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);

    int actfd = tf_accept4(listeningSocket, (sockaddr *)&addr, &addrlen, SOCK_CLOEXEC | SOCK_NONBLOCK);
    int err = errno;
    if (Q_UNLIKELY(actfd < 0)) {
        if (err != EAGAIN) {
            tSystemWarn("Failed accept.  errno:%d", err);
        }
        return NULL;
    }

    return create(actfd, QHostAddress((sockaddr *)&addr));
}


TEpollSocket *TEpollSocket::create(int socketDescriptor, const QHostAddress &address)
{
    TEpollSocket *sock = 0;

    if (Q_LIKELY(socketDescriptor > 0)) {
        sock  = new TEpollHttpSocket(socketDescriptor, address);
        sock->moveToThread(Tf::app()->thread());

        initBuffer(socketDescriptor);
    }

    return sock;
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
    const int BUF_SIZE = 128 * 1024;

    if (Q_UNLIKELY(sendBufSize == 0)) {
        // Creates a common buffer
        int res;
        socklen_t optlen;

        optlen = sizeof(int);
        res = getsockopt(socketDescriptor, SOL_SOCKET, SO_SNDBUF, &sendBufSize, &optlen);
        if (res < 0)
            sendBufSize = BUF_SIZE;

        optlen = sizeof(int);
        res = getsockopt(socketDescriptor, SOL_SOCKET, SO_RCVBUF, &recvBufSize, &optlen);
        if (res < 0)
            recvBufSize = BUF_SIZE;

    }
}


TEpollSocket::TEpollSocket(int socketDescriptor, const QHostAddress &address)
    : deleting(false), myWorkerCounter(0), pollIn(false), pollOut(false),
      sd(socketDescriptor), uuid(), clientAddr(address)
{
    uuid = QUuid::createUuid().toByteArray();  // not thread safe
    uuid = uuid.mid(1, uuid.length() - 2);
    tSystemDebug("TEpollSocket  id:%s", uuid.data());
}


TEpollSocket::~TEpollSocket()
{
    tSystemDebug("TEpollSocket::destructor");

    close();

    for (auto p : sendBuf) {
        delete p;
    }
    sendBuf.clear();
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
        len = tf_recv(sd, buf, recvBufSize, 0);
        err = errno;

        if (len <= 0) {
            break;
        }

        // Read successfully
        seekRecvBuffer(len);
    }

    if (!len && !err) {
        tSystemDebug("Socket disconnected : sd:%d", sd);
        ret = -1;
    } else {
        if (len < 0 || err > 0) {
            switch (err) {
            case EAGAIN:
                break;

            case ECONNRESET:
                tSystemDebug("Socket disconnected : sd:%d  errno:%d", sd, err);
                ret = -1;
                break;

            default:
                tSystemError("Failed recv : sd:%d  errno:%d  len:%d", sd, err, len);
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
    if (sendBuf.isEmpty()) {
        pollOut = true;
        return 0;
    }
    pollOut = false;

    if (deleting.load()) {
        return 0;
    }

    int ret = 0;
    int err = 0;
    int len;

    while (!sendBuf.isEmpty()) {
        TSendBuffer *buf = sendBuf.first();
        TAccessLogger &logger = buf->accessLogger();

        err = 0;
        for (;;) {
            len = sendBufSize;
            void *data = buf->getData(len);
            if (len == 0) {
                break;
            }

            errno = 0;
            len = tf_send(sd, data, len, MSG_NOSIGNAL);
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
            delete sendBuf.dequeue(); // delete send-buffer obj
        }

        if (len < 0) {
            switch (err) {
            case EAGAIN:
                break;

            case EPIPE:   // FALL THROUGH
            case ECONNRESET:
                tSystemDebug("Socket disconnected : sd:%d  errno:%d", sd, err);
                logger.setResponseBytes(-1);
                ret = -1;
                break;

            default:
                tSystemError("Failed send : sd:%d  errno:%d  len:%d", sd, err, len);
                logger.setResponseBytes(-1);
                ret = -1;
                break;
            }

            break;
        }
    }
    return ret;
}


void TEpollSocket::enqueueSendData(TSendBuffer *buffer)
{
    sendBuf << buffer;
}


void TEpollSocket::setSocketDescpriter(int socketDescriptor)
{
    sd = socketDescriptor;
}


void TEpollSocket::close()
{
    if (sd > 0) {
        tf_close(sd);
        sd = 0;
    }
}


void TEpollSocket::sendData(const QByteArray &header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger)
{
    if (!deleting.load()) {
        TEpoll::instance()->setSendData(this, header, body, autoRemove, accessLogger);
    }
}


void TEpollSocket::sendData(const QByteArray &data)
{
    if (!deleting.load()) {
        TEpoll::instance()->setSendData(this, data);
    }
}


void TEpollSocket::disconnect()
{
    if (!deleting.load())
        TEpoll::instance()->setDisconnect(this);
}


void TEpollSocket::switchToWebSocket(const THttpRequestHeader &header)
{
    if (!deleting.load())
        TEpoll::instance()->setSwitchToWebSocket(this, header);
}


qint64 TEpollSocket::bufferedBytes() const
{
    qint64 ret = 0;
    for (auto &d : sendBuf) {
        ret += d->arrayBuffer.size();
        if (d->bodyFile) {
            ret += d->bodyFile->size();
        }
    }
    return ret;
}


int TEpollSocket::bufferedListCount() const
{
    return sendBuf.count();
}


void TEpollSocket::deleteLater()
{
    tSystemDebug("TEpollSocket::deleteLater  countWorker:%d", (int)myWorkerCounter);
    deleting = true;
    if ((int)myWorkerCounter == 0) {
        QObject::deleteLater();
    }
}
