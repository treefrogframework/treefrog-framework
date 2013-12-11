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
#include <QMutex>
#include <QMutexLocker>
#include <TSystemGlobal>
#include <THttpHeader>
#include <TAtomicQueue>
#include "tepollsocket.h"
#include "tepoll.h"
#include "tactionworker2.h"
#include "thttpsendbuffer.h"
#include "tfcore_unix.h"

class SendData;

static char *tmpbuf = 0;
static int tmpbuflen = 0;
static QAtomicInt socketCounter;
static QMutex mutexSocketSet(QMutex::Recursive);
static QMap<int, TEpollSocket *> epollSockets;
static TAtomicQueue<SendData *> sendRequests;


class SendData
{
public:
    enum Method {
        Disconnect,
        Send,
    };
    int method;
    int id;
    THttpSendBuffer *buffer;

    SendData(Method m, int i, THttpSendBuffer *buf = 0) : method(m), id(i), buffer(buf) { }
};


TEpollSocket *TEpollSocket::create(int socketDescriptor, const QHostAddress &address)
{
    TEpollSocket *sock = 0;

    if (socketDescriptor > 0) {
        int id = socketCounter.fetchAndAddOrdered(1);
        sock  = new TEpollSocket(socketDescriptor, id, address);
        sock->moveToThread(QCoreApplication::instance()->thread());

        QMutexLocker locker(&mutexSocketSet);
        epollSockets.insert(id, sock);

        initBuffer(socketDescriptor);
    }

    return sock;
}


void TEpollSocket::initBuffer(int socketDescriptor)
{
    const int BUF_SIZE = 128 * 1024;

    if (!tmpbuf) {
        // Creates a common buffer
        int res, sendBufSize, recvBufSize;
        socklen_t optlen;

        optlen = sizeof(int);
        res = getsockopt(socketDescriptor, SOL_SOCKET, SO_SNDBUF, &sendBufSize, &optlen);
        if (res < 0)
            sendBufSize = BUF_SIZE;

        optlen = sizeof(int);
        res = getsockopt(socketDescriptor, SOL_SOCKET, SO_RCVBUF, &recvBufSize, &optlen);
        if (res < 0)
            recvBufSize = BUF_SIZE;

        tmpbuflen = qMax(sendBufSize, recvBufSize);
        tmpbuf = new char[tmpbuflen];
    }
}


void TEpollSocket::releaseAllSockets()
{
    QMutexLocker locker(&mutexSocketSet);

    for (QMapIterator<int, TEpollSocket *> it(epollSockets); it.hasNext(); ) {
        it.next();
        it.value()->deleteLater();
    }
    epollSockets.clear();
}


TEpollSocket::TEpollSocket(int socketDescriptor, int id, const QHostAddress &address)
    : sd(socketDescriptor), identifier(id), recvBuf(), sendBuf()
{
    recvBuf.setClientAddress(address);
}


TEpollSocket::~TEpollSocket()
{
    close();

    for (QListIterator<THttpSendBuffer*> it(sendBuf); it.hasNext(); ) {
        delete it.next();
    }
    sendBuf.clear();

    QMutexLocker locker(&mutexSocketSet);
    epollSockets.remove(identifier);
}


int TEpollSocket::recv()
{
    int err;

    for (;;) {
        errno = 0;
        int len = ::recv(sd, tmpbuf, tmpbuflen, 0);
        err = errno;

        if (len <= 0) {
            break;
        }

        // Read successfully
        recvBuf.write(tmpbuf, len);
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

    THttpSendBuffer *sendbuf = sendBuf.first();
    int len = sendbuf->read(tmpbuf, tmpbuflen);

    errno = 0;
    int sentlen = ::send(sd, tmpbuf, len, 0);
    int err = errno;
    TAccessLogger &logger = sendbuf->accessLogger();

    if (sentlen <= 0) {
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

        if (len > sentlen) {
            tSystemDebug("sendbuf prepend: len:%d", len - sentlen);
            sendbuf->prepend(tmpbuf + sentlen, len - sentlen);
        }

        if (sendbuf->atEnd()) {
            logger.write();  // Writes access log
            sendbuf->release();

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


bool TEpollSocket::canReadHttpRequest()
{
    return recvBuf.canReadHttpRequest();
}


bool TEpollSocket::waitSendData(int msec)
{
    return sendRequests.wait(msec);
}


void TEpollSocket::dispatchSendData()
{
    QList<SendData *> dataList = sendRequests.dequeue();

    for (QListIterator<SendData *> it(dataList); it.hasNext(); ) {
        SendData *sd = it.next();
        TEpollSocket *sock = epollSockets[sd->id];

        if (sock && sock->socketDescriptor() > 0) {
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


void TEpollSocket::setSendData(int id, const THttpHeader *header, QIODevice *body, bool autoRemove, const TAccessLogger &accessLogger)
{
    QByteArray response = header->toByteArray();
    QFileInfo fi;

    if (body) {
        QBuffer *buffer = qobject_cast<QBuffer *>(body);
        if (buffer) {
            response += buffer->data();
        } else {
            fi.setFile(*qobject_cast<QFile *>(body));
        }
    }

    THttpSendBuffer *sendbuf = new THttpSendBuffer(response, fi, autoRemove, accessLogger);
    sendRequests.enqueue(new SendData(SendData::Send, id, sendbuf));
}


void TEpollSocket::setDisconnect(int id)
{
    sendRequests.enqueue(new SendData(SendData::Disconnect, id));
}


void TEpollSocket::close()
{
    if (sd > 0) {
        TF_CLOSE(sd);
        sd = 0;
    }
}


void TEpollSocket::startWorker()
{
    TActionWorker *worker = new TActionWorker(this);
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    worker->start();
}
