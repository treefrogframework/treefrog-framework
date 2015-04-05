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
    tSystemDebug("readBinaryRequest: %s", ret.data());
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
        TWebSocketWorker *worker = new TWebSocketWorker(this, reqHeader.path(), opcode, binary);
        worker->moveToThread(Tf::app()->thread());
        connect(worker, SIGNAL(finished()), this, SLOT(releaseWorker()));
        myWorkerCounter.fetchAndAddOrdered(1); // count-up
        worker->start();
    } while (canReadRequest());
}


void TEpollWebSocket::releaseWorker()
{
    tSystemDebug("TEpollWebSocket::releaseWorker");
    TWebSocketWorker *worker = dynamic_cast<TWebSocketWorker *>(sender());
    if (worker) {
        worker->deleteLater();
        myWorkerCounter.fetchAndAddOrdered(-1);  // count-down

        if (deleting) {
            TEpollSocket::deleteLater();
        }
    }
}


void TEpollWebSocket::startWorkerForOpening(const TSession &session)
{
    TWebSocketWorker *worker = new TWebSocketWorker(this, session);
    worker->moveToThread(Tf::app()->thread());
    connect(worker, SIGNAL(finished()), this, SLOT(releaseWorker()));
    myWorkerCounter.fetchAndAddOrdered(1); // count-up
    worker->start();
}


void TEpollWebSocket::clear()
{
    recvBuffer.resize(BUFFER_RESERVE_SIZE);
    recvBuffer.squeeze();
    recvBuffer.truncate(0);
    frames.clear();
}


void TEpollWebSocket::sendText(const QString &message)
{
    TEpollWebSocket::sendText(this, message);
}


void TEpollWebSocket::sendBinary(const QByteArray &data)
{
    TEpollWebSocket::sendBinary(this, data);
}


void TEpollWebSocket::sendPing()
{
    TEpollWebSocket::sendPing(this);
}


void TEpollWebSocket::sendPong()
{
    TEpollWebSocket::sendPong(this);
}


void TEpollWebSocket::sendText(TEpollSocket *socket, const QString &message)
{
    TWebSocketFrame frame;
    frame.setOpCode(TWebSocketFrame::TextFrame);
    frame.setPayload(message.toUtf8());
    socket->sendData(frame.toByteArray());
}


void TEpollWebSocket::sendBinary(TEpollSocket *socket, const QByteArray &data)
{
    TWebSocketFrame frame;
    frame.setOpCode(TWebSocketFrame::BinaryFrame);
    frame.setPayload(data);
    socket->sendData(frame.toByteArray());
}


void TEpollWebSocket::sendPing(TEpollSocket *socket)
{
    TWebSocketFrame frame;
    frame.setOpCode(TWebSocketFrame::Ping);
    socket->sendData(frame.toByteArray());
}


void TEpollWebSocket::sendPong(TEpollSocket *socket)
{
    TWebSocketFrame frame;
    frame.setOpCode(TWebSocketFrame::Pong);
    socket->sendData(frame.toByteArray());
}

