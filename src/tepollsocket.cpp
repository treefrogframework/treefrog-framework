/* Copyright (c) 2013-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tepollsocket.h"
#include "tepoll.h"
#include "tfcore.h"
#include "tsendbuffer.h"
#include <THttpHeader>
#include <TSystemGlobal>
#include <TWebApplication>
#include <TMultiplexingServer>
#include <QFileInfo>
#include <QSet>

class SendData;

union tf_sockaddr {
    sockaddr a;
    sockaddr_in a4;
    sockaddr_in6 a6;
};


namespace {
int sendBufSize = 0;
int recvBufSize = 0;
QSet<TEpollSocket *> socketManager;


void setAddressAndPort(const QHostAddress &address, uint16_t port, tf_sockaddr *aa, int &addrSize)
{
    if (address.protocol() == QAbstractSocket::IPv6Protocol || address.protocol() == QAbstractSocket::AnyIPProtocol) {
        std::memset(&aa->a6, 0, sizeof(sockaddr_in6));
        aa->a6.sin6_family = AF_INET6;
        //aa->a6.sin6_scope_id = QNetworkInterface::interfaceIndexFromName(address.scopeId());
        aa->a6.sin6_port = htons(port);
        auto tmp = address.toIPv6Address();
        std::memcpy(&aa->a6.sin6_addr, &tmp, sizeof(tmp));
        addrSize = sizeof(sockaddr_in6);
    } else {
        std::memset(&aa->a4, 0, sizeof(sockaddr_in));
        aa->a4.sin_family = AF_INET;
        aa->a4.sin_port = htons(port);
        aa->a4.sin_addr.s_addr = htonl(address.toIPv4Address());
        addrSize = sizeof(sockaddr_in);
    }
}

}

TSendBuffer *TEpollSocket::createSendBuffer(const QByteArray &header, const QFileInfo &file, bool autoRemove, TAccessLogger &&logger)
{
    return new TSendBuffer(header, file, autoRemove, std::move(logger));
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


TEpollSocket::TEpollSocket()
{
    _socket = ::socket(AF_INET, (SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK), 0);
    tSystemDebug("TEpollSocket  socket:{}", _socket);
    socketManager.insert(this);
    initBuffer(_socket);
}


TEpollSocket::TEpollSocket(int socketDescriptor, Tf::SocketState state, const QHostAddress &peerAddress) :
    _socket(socketDescriptor),
    _state(state),
    _peerAddress(peerAddress)
{
    tSystemDebug("TEpollSocket  socket:{}", _socket);
    socketManager.insert(this);
    initBuffer(_socket);
}


TEpollSocket::~TEpollSocket()
{
    tSystemDebug("TEpollSocket::destructor");

    close();
    socketManager.remove(this);
    TMultiplexingServer::instance()->_garbageSockets.remove(this);

    while (!_sendBuffer.isEmpty()) {
        TSendBuffer *buf = _sendBuffer.dequeue();
        delete buf;
    }
}

// Disposes for gabage collection
void TEpollSocket::dispose()
{
    close();
    if (autoDelete()) {
        TMultiplexingServer::instance()->_garbageSockets.insert(this);
    }
}


void TEpollSocket::close()
{
    if (_socket > 0) {
        tf_close_socket(_socket);
        _socket = 0;
    }
    _state = Tf::SocketState::Unconnected;
}


void TEpollSocket::connectToHost(const QHostAddress &address, uint16_t port)
{
    tf_sockaddr aa;
    int addrSize;
    setAddressAndPort(address, port, &aa, addrSize);

    int res = tf_connect(_socket, (struct sockaddr *)&aa.a, addrSize);
    if (res < 0 && errno != EINPROGRESS) {
        tSystemError("Failed connect");
        close();
        return;
    }

    tSystemDebug("TCP connection state: {}", res);
    _state = (res == 0) ? Tf::SocketState::Connected : Tf::SocketState::Connecting;
    _peerAddress = address;
    watch();
}


bool TEpollSocket::watch()
{
    bool ret = false;

    switch (state()) {
    case Tf::SocketState::Unconnected:
        break;

    case Tf::SocketState::Connecting:  // fall through
    case Tf::SocketState::Connected:
        ret = TEpoll::instance()->addPoll(this, (EPOLLIN | EPOLLOUT | EPOLLET));
        if (!ret) {
            close();
        }
        break;

    default:
        tSystemError("Logic error [{}:{}]", __FILE__, __LINE__);
        break;
    }
    return ret;
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
        len = tf_recv(_socket, buf, recvBufSize, 0);
        err = errno;

        if (len <= 0) {
            break;
        }

        // Read successfully
        seekRecvBuffer(len);
    }

    if (!len && !err) {
        tSystemDebug("Socket disconnected : sd:{}", _socket);
        ret = -1;
    } else {
        if (len < 0 || err > 0) {
            switch (err) {
            case EAGAIN:
                break;

            case ECONNRESET:
                tSystemDebug("Socket disconnected : sd:{}  errno:{}", _socket, err);
                ret = -1;
                break;

            default:
                tSystemError("Failed recv : sd:{}  errno:{}  len:{}", _socket, err, len);
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
        return ret;
    }

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
            len = tf_send(_socket, data, len);
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
                tSystemDebug("Socket disconnected : sd:{}  errno:{}", _socket, err);
                logger.setResponseBytes(-1);
                ret = -1;
                break;

            default:
                tSystemError("Failed send : sd:{}  errno:{}  len:{}", _socket, err, len);
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


bool TEpollSocket::setSocketOption(int level, int optname, int val)
{
    if (_socket < 1) {
        tSystemError("Logic error [{}:{}]", __FILE__, __LINE__);
        return false;
    }

    int res = ::setsockopt(_socket, level, optname, &val, sizeof(val));
    if (res < 0) {
        tSystemError("setsockopt error: {}  [{}:{}]", res, __FILE__, __LINE__);
    }
    return !res;
}


void TEpollSocket::enqueueSendData(TSendBuffer *buffer)
{
    _sendBuffer.enqueue(buffer);
}


void TEpollSocket::setSocketDescriptor(int socketDescriptor)
{
    _socket = socketDescriptor;
}


void TEpollSocket::sendData(const QByteArray &header, QIODevice *body, bool autoRemove, TAccessLogger &&accessLogger)
{
    TEpoll::instance()->setSendData(this, header, body, autoRemove, std::move(accessLogger));
}


void TEpollSocket::sendData(const QByteArray &data)
{
    TEpoll::instance()->setSendData(this, data);
}


int64_t TEpollSocket::receiveData(char *buffer, int64_t length)
{
    int64_t len = std::min(length, (int64_t)_recvBuffer.length());
    if (len > 0) {
        std::memcpy(buffer, _recvBuffer.data(), len);
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


bool TEpollSocket::waitUntil(bool (TEpollSocket::*method)(), int msecs)
{
    QElapsedTimer elapsed;
    elapsed.start();

    while (!(this->*method)()) {
        int ms = msecs - elapsed.elapsed();
        if (ms <= 0) {
            break;
        }

        if (TMultiplexingServer::instance()->processEvents(ms) < 0) {
            break;
        }
    }
    return (this->*method)();
}


bool TEpollSocket::waitForConnected(int msecs)
{
    return waitUntil((bool (TEpollSocket::*)())&TEpollSocket::isConnected, msecs);
}


bool TEpollSocket::waitForDataSent(int msecs)
{
    return waitUntil((bool (TEpollSocket::*)())&TEpollSocket::isDataSent, msecs);
}


bool TEpollSocket::waitForDataReceived(int msecs)
{
    return waitUntil((bool (TEpollSocket::*)())&TEpollSocket::isDataReceived, msecs);
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
