/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tepollwebsocket.h"
#include "tdispatcher.h"
#include "tepoll.h"
#include "turlroute.h"
#include "twebsocketframe.h"
#include "twebsocketworker.h"
#include <QCryptographicHash>
#include <QDataStream>
#include <TAppSettings>
#include <THttpRequestHeader>
#include <THttpUtility>
#include <TSystemGlobal>
#include <TWebApplication>

constexpr int BUFFER_RESERVE_SIZE = 127;

namespace {
QMap<int, TEpollWebSocket *> socketManager;
}


TEpollWebSocket::TEpollWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header) :
    QObject(),
    TEpollSocket(socketDescriptor, Tf::SocketState::Connected, address),
    TAbstractWebSocket(header)
{
    tSystemDebug("TEpollWebSocket  [%p]", this);
    socketManager.insert(socketDescriptor, this);
    _recvBuffer.reserve(BUFFER_RESERVE_SIZE);
}


TEpollWebSocket::~TEpollWebSocket()
{
    socketManager.remove(socketDescriptor());
    tSystemDebug("~TEpollWebSocket  [%p]", this);
}


bool TEpollWebSocket::canReadRequest()
{
    for (auto &frm : (const QList<TWebSocketFrame> &)_frames) {
        if (frm.isFinalFrame() && frm.state() == TWebSocketFrame::Completed) {
            return true;
        }
    }
    return false;
}


bool TEpollWebSocket::isTextRequest() const
{
    if (!_frames.isEmpty()) {
        const TWebSocketFrame &frm = _frames.first();
        return (frm.opCode() == TWebSocketFrame::TextFrame);
    }
    return false;
}


bool TEpollWebSocket::isBinaryRequest() const
{
    if (!_frames.isEmpty()) {
        const TWebSocketFrame &frm = _frames.first();
        return (frm.opCode() == TWebSocketFrame::BinaryFrame);
    }
    return false;
}


void TEpollWebSocket::sendTextForPublish(const QString &text, const QObject *except)
{
    tSystemDebug("sendText  text len:%ld  (pid:%d)", (int64_t)text.length(), (int)QCoreApplication::applicationPid());
    if (except != this) {
        TAbstractWebSocket::sendText(text);
    }
}


void TEpollWebSocket::sendBinaryForPublish(const QByteArray &binary, const QObject *except)
{
    tSystemDebug("sendBinary  binary len:%ld  (pid:%d)", (int64_t)binary.length(), (int)QCoreApplication::applicationPid());
    if (except != this) {
        TAbstractWebSocket::sendBinary(binary);
    }
}


void TEpollWebSocket::sendPong(const QByteArray &data)
{
    tSystemDebug("sendPong  data len:%ld  (pid:%d)", (int64_t)data.length(), (int)QCoreApplication::applicationPid());
    TAbstractWebSocket::sendPong(data);
}


QList<QPair<int, QByteArray>> TEpollWebSocket::readAllBinaryRequest()
{
    Q_ASSERT(canReadRequest());
    QList<QPair<int, QByteArray>> ret;
    QByteArray payload;

    while (canReadRequest()) {
        int opcode = _frames.first().opCode();
        payload.resize(0);

        while (!_frames.isEmpty()) {
            TWebSocketFrame frm = _frames.takeFirst();
            payload += frm.payload();
            if (frm.isFinalFrame() && frm.state() == TWebSocketFrame::Completed) {
                ret << qMakePair(opcode, payload);
                break;
            }
        }
    }
    return ret;
}


bool TEpollWebSocket::seekRecvBuffer(int pos)
{
    int size = _recvBuffer.size();
    if (Q_UNLIKELY(pos <= 0 || size + pos > _recvBuffer.capacity())) {
        Q_ASSERT(0);
        return false;
    }

    size += pos;
    _recvBuffer.resize(size);
    int len = parse(_recvBuffer);
    tSystemDebug("WebSocket parse len : %d", len);
    if (len < 0) {
        tSystemError("WebSocket parse error [%s:%d]", __FILE__, __LINE__);
        close();
        return false;
    }
    return true;
}


void TEpollWebSocket::process()
{
    tSystemDebug("TEpollWebSocket::process");
    Q_ASSERT(canReadRequest());

    auto payloads = readAllBinaryRequest();
    if (!payloads.isEmpty()) {
        TWebSocketWorker *_worker = new TWebSocketWorker(TWebSocketWorker::Receiving, this, reqHeader.path());
        _worker->setPayloads(payloads);
        startWorker(_worker);
        delete _worker;
        _worker = nullptr;
        releaseWorker();
    }
}


void TEpollWebSocket::startWorker(TWebSocketWorker *worker)
{
    worker->moveToThread(thread());
    worker->start();
    worker->wait();
}


void TEpollWebSocket::releaseWorker()
{
    tSystemDebug("TEpollWebSocket::releaseWorker");

    bool res = TEpoll::instance()->modifyPoll(this, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
    if (!res) {
        dispose();
    }
}


void TEpollWebSocket::startWorkerForOpening(const TSession &session)
{
    TWebSocketWorker *worker = new TWebSocketWorker(TWebSocketWorker::Opening, this, reqHeader.path());
    worker->setSession(session);
    startWorker(worker);

    releaseWorker();
    delete worker;
}


void TEpollWebSocket::startWorkerForClosing()
{
    if (!closing.load()) {
        TWebSocketWorker *worker = new TWebSocketWorker(TWebSocketWorker::Closing, this, reqHeader.path());
        startWorker(worker);

        releaseWorker();
        delete worker;
    }
}


void TEpollWebSocket::clear()
{
    _recvBuffer.resize(BUFFER_RESERVE_SIZE);
    _recvBuffer.squeeze();
    _recvBuffer.truncate(0);
    _frames.clear();
}


int64_t TEpollWebSocket::writeRawData(const QByteArray &data)
{
    sendData(data);
    return data.length();
}


void TEpollWebSocket::disconnect()
{
    TEpollSocket::disconnect();
    stopKeepAlive();
}


void TEpollWebSocket::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == keepAliveTimer->timerId()) {
        sendPing();
    } else {
        QObject::timerEvent(event);
    }
}


TEpollWebSocket *TEpollWebSocket::searchSocket(int socket)
{
    return socketManager.value(socket, nullptr);
}
