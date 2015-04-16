/* Copyright (c) 2015, AOYAMA Kazuharu
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
#include <TWebSocketEndpoint>
#include "tepollwebsocket.h"
#include "twebsocketframe.h"
#include "twebsocketworker.h"
#include "turlroute.h"
#include "tdispatcher.h"

const int BUFFER_RESERVE_SIZE = 127;


TEpollWebSocket::TEpollWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header)
    : TEpollSocket(socketDescriptor, address), TAbstractWebSocket(),
      reqHeader(header), recvBuffer(), frames()
{
    recvBuffer.reserve(BUFFER_RESERVE_SIZE);
}


TEpollWebSocket::~TEpollWebSocket()
{
    tSystemDebug("~TEpollWebSocket");
}


bool TEpollWebSocket::canReadRequest()
{
    for (auto &frm : frames) {
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


void TEpollWebSocket::sendText(const QString &text)
{
    tSystemDebug("sendText  text len:%d  (pid:%d)", text.length(), (int)QCoreApplication::applicationPid());
    TAbstractWebSocket::sendText(text);
}


void TEpollWebSocket::sendBinary(const QByteArray &binary)
{
    TAbstractWebSocket::sendBinary(binary);
}


QByteArray TEpollWebSocket::readBinaryRequest()
{
    Q_ASSERT(canReadRequest());

    QByteArray ret;
    while (!frames.isEmpty()) {
        TWebSocketFrame frm = frames.takeFirst();
        ret += frm.payload();
        if (frm.isFinalFrame() && frm.state() == TWebSocketFrame::Completed) {
            break;
        }
    }
    tSystemDebug("readBinaryRequest: payload len:%d", ret.length());
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
        clear();
        return false;
    }

    size += pos;
    recvBuffer.resize(size);
    int len = parse(recvBuffer);
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

    do {
        TWebSocketFrame::OpCode opcode = frames.first().opCode();
        QByteArray binary = readBinaryRequest();
        TWebSocketWorker *worker = new TWebSocketWorker(TWebSocketWorker::Receiving, this, reqHeader.path());
        worker->setPayload(opcode, binary);
        startWorker(worker);
    } while (canReadRequest());
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
        }
    }
}


void TEpollWebSocket::deleteLater()
{
    tSystemDebug("TEpollWebSocket::deleteLater  countWorkers:%d", (int)myWorkerCounter);
    if (!deleting.load()) {
        startWorkerForClosing();
    }

    TEpollSocket::deleteLater();
}


void TEpollWebSocket::startWorkerForOpening(const TSession &session)
{
    TWebSocketWorker *worker = new TWebSocketWorker(TWebSocketWorker::Opening, this, reqHeader.path());
    worker->setSession(session);
    startWorker(worker);
}


void TEpollWebSocket::startWorkerForClosing()
{
    if (!TAbstractWebSocket::closing.exchange(true)) {
        TWebSocketWorker *worker = new TWebSocketWorker(TWebSocketWorker::Closing, this, reqHeader.path());
        startWorker(worker);
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
}
