/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <sys/types.h>
#include <sys/epoll.h>
#include <QCoreApplication>
#include <QFileInfo>
#include <QBuffer>
#include <QDateTime>
#include <TSystemGlobal>
#include <THttpHeader>
#include <TAtomicQueue>
#include "tepollsocket.h"
#include "tepollhttpsocket.h"
#include "tepoll.h"
#include "thttpsendbuffer.h"
#include "tfcore_unix.h"

class SendData;

static int sendBufSize = 0;
static int recvBufSize = 0;
static QAtomicInt objectCounter(1);


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
        sock->moveToThread(QCoreApplication::instance()->thread());

        initBuffer(socketDescriptor);
    }

    return sock;
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
    : sd(socketDescriptor), identifier(0), clientAddr(address)
{
    quint64 h = QDateTime::currentDateTime().toTime_t();
    quint64 b = objectCounter.fetchAndAddOrdered(1);
    identifier = (h << 32) | (b & 0xffffffff);
    tSystemDebug("TEpollSocket  id:%llu", identifier);
}


TEpollSocket::~TEpollSocket()
{
    close();

    for (QListIterator<THttpSendBuffer*> it(sendBuf); it.hasNext(); ) {
        delete it.next();
    }
    sendBuf.clear();
}


int TEpollSocket::recv()
{
    int err;

    for (;;) {
#if 0
        errno = 0;
        int len = ::recv(sd, tmpbuf, tmpbuflen, 0);
        err = errno;

        if (len <= 0) {
            break;
        }

        // Read successfully
        recvBuf.write(tmpbuf, len);
#else
        void *buf = getRecvBuffer(recvBufSize);
        errno = 0;
        int len = ::recv(sd, buf, recvBufSize, 0);
        err = errno;

        if (len <= 0) {
            break;
        }

        // Read successfully
        seekRecvBuffer(len);
#endif
    }

    switch (err) {
    case EAGAIN:
        break;

    case 0:  // FALL THROUGH
    case ECONNRESET:
        tSystemDebug("Socket disconnected : errno:%d", err);
        return -1;
        break;

    default:
        tSystemError("Failed recv : errno:%d", err);
        return -1;
        break;
    }
    return 0;
}


int TEpollSocket::send()
{
    if (sendBuf.isEmpty()) {
        return 0;
    }

    THttpSendBuffer *buf = sendBuf.first();
#if 0
    int len = sendbuf->read(tmpbuf, tmpbuflen);
    errno = 0;
    int sentlen = ::send(sd, tmpbuf, len, 0);
    int err = errno;
#else
    int len = sendBufSize;
    void *data = buf->getData(len);
    if (Q_UNLIKELY(len == 0)) {
        buf->release();
        delete sendBuf.dequeue(); // delete send-buffer obj
        return 0;
    }

    errno = 0;
    int sentlen = ::send(sd, data, len, 0);
    int err = errno;
#endif
    TAccessLogger &logger = buf->accessLogger();

    if (Q_UNLIKELY(sentlen <= 0)) {
        if (err != ECONNRESET) {
            tSystemError("Failed send : errno:%d  datalen:%d", err, len);
        }
        // Access log
        logger.setResponseBytes(-1);
        logger.write();
        return -1;

    } else {
        // Sent successfully
        logger.setResponseBytes(logger.responseBytes() + sentlen);
        buf->seekData(sentlen);

        if (buf->atEnd()) {
            logger.write();  // Writes access log
            buf->release();

            delete sendBuf.dequeue(); // delete send-buffer obj

#if 1  //TODO: delete here for HTTP 2.0 support
            if (sendBuf.isEmpty()) {
                TEpoll::instance()->modifyPoll(this, (EPOLLIN | EPOLLOUT | EPOLLET));
            }
#endif
        }
    }

    return sentlen;
}

/*
bool TEpollSocket::waitSendData(int msec)
{
    return sendRequests.wait(msec);
}


void TEpollSocket::dispatchSendData()
{
    QList<SendData *> dataList = sendRequests.dequeue();

    for (QListIterator<SendData *> it(dataList); it.hasNext(); ) {
        SendData *sd = it.next();

        TEpollSocket *sock = sd->socket;

        if (Q_LIKELY(sock && sock->socketDescriptor() > 0)) {
            switch (sd->method) {
            case SendData::Send:
                sock->sendBuf << sd->buffer;
                TEpoll::instance()->modifyPoll(sock, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
                break;

            case SendData::Disconnect:
                TEpoll::instance()->deletePoll(sock);
                sock->close();
                sock->deleteLater();
                break;

	    case SendData::SwitchProtocols:
                TEpoll::instance()->deletePoll(sock);
                sock->setSocketDescpriter(0);
                sock->deleteLater();
    tSystemDebug("##### SwitchProtocols");
                // Switching protocols
                sd->upgradedSocket->sendBuf << sd->buffer;
                TEpoll::instance()->addPoll(sd->upgradedSocket, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
                break;

            default:
                tSystemError("Logic error [%s:%d]", __FILE__, __LINE__);
                if (sd->buffer) {
                    delete sd->buffer;
                }
                break;
            }
        }

        delete sd;
    }
}
*/

/*
void TEpollSocket::setSendData(const QByteArray &header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger)
{
    QByteArray response = header;
    QFileInfo fi;

    if (Q_LIKELY(body)) {
        QBuffer *buffer = qobject_cast<QBuffer *>(body);
        if (buffer) {
            response += buffer->data();
        } else {
            fi.setFile(*qobject_cast<QFile *>(body));
        }
    }

    THttpSendBuffer *sendbuf = new THttpSendBuffer(response, fi, autoRemove, accessLogger);
    sendRequests.enqueue(new SendData(SendData::Send, this, sendbuf));
}


void TEpollSocket::setDisconnect()
{
    sendRequests.enqueue(new SendData(SendData::Disconnect, this));
}


void TEpollSocket::setSwitchProtocols(const QByteArray &header, TEpollSocket *target)
{
    THttpSendBuffer *sendbuf = new THttpSendBuffer(header);
    sendRequests.enqueue(new SendData(SendData::SwitchProtocols, this, sendbuf, target));
}
*/

void TEpollSocket::setSocketDescpriter(int socketDescriptor)
{
    sd = socketDescriptor;
}


void TEpollSocket::close()
{
    if (sd > 0) {
        TF_CLOSE(sd);
        sd = 0;
    }
}
