/* Copyright (c) 2015-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDataStream>
#include <QCryptographicHash>
#include <TWebApplication>
#include <TSystemGlobal>
#include <TAppSettings>
#include <THttpRequestHeader>
#include <THttpUtility>
#include "tepollwebsocket.h"
#include "tepoll.h"
#include "twebsocketframe.h"
#include "twebsocketworker.h"
#include "turlroute.h"
#include "tdispatcher.h"

const int BUFFER_RESERVE_SIZE = 127;


TEpollWebSocket::TEpollWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header)
    : TEpollSocket(socketDescriptor, address), TAbstractWebSocket(header),
      recvBuffer(), frames()
{
    tSystemDebug("TEpollWebSocket  [%p]", this);
    recvBuffer.reserve(BUFFER_RESERVE_SIZE);
}


TEpollWebSocket::~TEpollWebSocket()
{
    tSystemDebug("~TEpollWebSocket  [%p]", this);
}


bool TEpollWebSocket::canReadRequest()
{
    for (auto &frm : (const QList<TWebSocketFrame>&)frames) {
        if (frm.isFinalFrame() && frm.state() == TWebSocketFrame::Completed) {
            return true;
        }
    }
    return false;
}


bool TEpollWebSocket::isTextRequest() const
{
    if (!frames.isEmpty()) {
        const TWebSocketFrame &frm = frames.first();
        return (frm.opCode() == TWebSocketFrame::TextFrame);
    }
    return false;
}


bool TEpollWebSocket::isBinaryRequest() const
{
    if (!frames.isEmpty()) {
        const TWebSocketFrame &frm = frames.first();
        return (frm.opCode() == TWebSocketFrame::BinaryFrame);
    }
    return false;
}


void TEpollWebSocket::sendTextForPublish(const QString &text, const QObject *except)
{
    tSystemDebug("sendText  text len:%d  (pid:%d)", text.length(), (int)QCoreApplication::applicationPid());
    if (except != this) {
        TAbstractWebSocket::sendText(text);
    }
}


void TEpollWebSocket::sendBinaryForPublish(const QByteArray &binary, const QObject *except)
{
    tSystemDebug("sendBinary  binary len:%d  (pid:%d)", binary.length(), (int)QCoreApplication::applicationPid());
    if (except != this) {
        TAbstractWebSocket::sendBinary(binary);
    }
}


void TEpollWebSocket::sendPong(const QByteArray &data)
{
    tSystemDebug("sendPong  data len:%d  (pid:%d)", data.length(), (int)QCoreApplication::applicationPid());
    TAbstractWebSocket::sendPong(data);
}


QList<QPair<int, QByteArray>> TEpollWebSocket::readAllBinaryRequest()
{
    Q_ASSERT(canReadRequest());
    QList<QPair<int, QByteArray>> ret;
    QByteArray payload;

    while (canReadRequest()) {
        int opcode = frames.first().opCode();
        payload.resize(0);

        while (!frames.isEmpty()) {
            TWebSocketFrame frm = frames.takeFirst();
            payload += frm.payload();
            if (frm.isFinalFrame() && frm.state() == TWebSocketFrame::Completed) {
                ret << qMakePair(opcode, payload);
                break;
            }
        }
    }
    return ret;
}


void *TEpollWebSocket::getRecvBuffer(int size)
{
    int len = recvBuffer.size();
    recvBuffer.reserve(len + size);
    return recvBuffer.data() + len;
}


bool TEpollWebSocket::seekRecvBuffer(int pos)
{
    int size = recvBuffer.size();
    if (Q_UNLIKELY(pos <= 0 || size + pos > recvBuffer.capacity())) {
        Q_ASSERT(0);
        return false;
    }

    size += pos;
    recvBuffer.resize(size);
    int len = parse(recvBuffer);
    tSystemDebug("WebSocket parse len : %d", len);
    if (len < 0) {
        tSystemError("WebSocket parse error [%s:%d]", __FILE__, __LINE__);
        close();
        return false;
    }
    return true;
}


void TEpollWebSocket::startWorker()
{
    tSystemDebug("TEpollWebSocket::startWorker");
    Q_ASSERT(canReadRequest());

    auto payloads = readAllBinaryRequest();
    if (!payloads.isEmpty()) {
        TWebSocketWorker *worker = new TWebSocketWorker(TWebSocketWorker::Receiving, this, reqHeader.path());
        worker->setPayloads(payloads);
        startWorker(worker);
    }
}


void TEpollWebSocket::startWorker(TWebSocketWorker *worker)
{
    worker->moveToThread(thread());
    connect(worker, SIGNAL(finished()), this, SLOT(releaseWorker()));
    ++myWorkerCounter; // count-up
    worker->start();
}


void TEpollWebSocket::releaseWorker()
{
    tSystemDebug("TEpollWebSocket::releaseWorker");
    TWebSocketWorker *worker = qobject_cast<TWebSocketWorker *>(sender());
    if (worker) {
        worker->deleteLater();
        --myWorkerCounter;  // count-down

        if (deleting.load()) {
            TEpollWebSocket::deleteLater();
        } else {
            if (pollIn.exchange(false)) {
                TEpoll::instance()->modifyPoll(this, (EPOLLIN | EPOLLOUT | EPOLLET));  // reset
            }
        }
    }
}


void TEpollWebSocket::deleteLater()
{
    tSystemDebug("TEpollWebSocket::deleteLater  countWorkers:%d  deleting:%d", (int)myWorkerCounter, (bool)deleting);

    if (!deleting.exchange(true)) {
        startWorkerForClosing();
        return;
    }

    if ((int)myWorkerCounter == 0) {
        QObject::deleteLater();
    }
}


void TEpollWebSocket::startWorkerForOpening(const TSession &session)
{
    TWebSocketWorker *worker = new TWebSocketWorker(TWebSocketWorker::Opening, this, reqHeader.path());
    worker->setSession(session);
    startWorker(worker);
}


void TEpollWebSocket::startWorkerForClosing()
{
    if (!closing.load()) {
        TWebSocketWorker *worker = new TWebSocketWorker(TWebSocketWorker::Closing, this, reqHeader.path());
        startWorker(worker);
    } else {
        deleteLater();
    }
}


void TEpollWebSocket::clear()
{
    recvBuffer.resize(BUFFER_RESERVE_SIZE);
    recvBuffer.squeeze();
    recvBuffer.truncate(0);
    frames.clear();
}


qint64 TEpollWebSocket::writeRawData(const QByteArray &data)
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
        TEpollSocket::timerEvent(event);
    }
}


TEpollWebSocket *TEpollWebSocket::searchSocket(int sid)
{
    TEpollSocket *sock = TEpollSocket::searchSocket(sid);
    return dynamic_cast<TEpollWebSocket*>(sock);
}
