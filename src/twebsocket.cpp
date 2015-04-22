/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QUuid>
#include <TWebSocketEndpoint>
#include <TWebApplication>
#include "twebsocket.h"
#include "twebsocketworker.h"
#include "turlroute.h"
#include "tdispatcher.h"

const qint64 WRITE_LENGTH = 1280;
const int BUFFER_RESERVE_SIZE = 127;


TWebSocket::TWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header, QObject *parent)
    : QTcpSocket(parent), frames(), uuid(), reqHeader(header), recvBuffer(),
      myWorkerCounter(0), deleting(false)
{
    setSocketDescriptor(socketDescriptor);
    setPeerAddress(address);

    uuid = QUuid::createUuid().toByteArray().replace('-', "");  // not thread safe
    uuid = uuid.mid(1, uuid.length() - 2);
    recvBuffer.reserve(BUFFER_RESERVE_SIZE);

    connect(this, SIGNAL(readyRead()), this, SLOT(readRequest()));
    connect(this, SIGNAL(sendByWorker(const QByteArray &)), this, SLOT(sendRawData(const QByteArray &)));
    connect(this, SIGNAL(disconnectByWorker()), this, SLOT(close()));
}


TWebSocket::~TWebSocket()
{
    tSystemDebug("~TWebSocket");
}


void TWebSocket::close()
{
    QTcpSocket::close();
}


void TWebSocket::sendText(const QString &text)
{
    tSystemDebug("sendText  text len:%d  (pid:%d)", text.length(), (int)QCoreApplication::applicationPid());
    TAbstractWebSocket::sendText(text);
}


void TWebSocket::sendBinary(const QByteArray &binary)
{
    tSystemDebug("sendBinary  binary len:%d  (pid:%d)", binary.length(), (int)QCoreApplication::applicationPid());
    TAbstractWebSocket::sendBinary(binary);
}


void TWebSocket::sendPong(const QByteArray &data)
{
    tSystemDebug("sendPong  data len:%d  (pid:%d)", data.length(), (int)QCoreApplication::applicationPid());
    TAbstractWebSocket::sendPong(data);
}


bool TWebSocket::canReadRequest() const
{
    for (const auto &frm : frames) {
        if (frm.isFinalFrame() && frm.state() == TWebSocketFrame::Completed) {
            return true;
        }
    }
    return false;
}


void TWebSocket::readRequest()
{
    qint64 bytes;
    QByteArray buf;

    while ((bytes = bytesAvailable()) > 0) {
        buf.resize(bytes);
        bytes = QTcpSocket::read(buf.data(), bytes);
        if (Q_UNLIKELY(bytes < 0)) {
            tSystemError("socket read error");
            break;
        }

        recvBuffer.append(buf.data(), bytes);
    }

    int len = parse(recvBuffer);
    if (len < 0) {
        tSystemError("WebSocket parse error [%s:%d]", __FILE__, __LINE__);
        disconnect();
        return;
    }

    QByteArray binary;
    while (!frames.isEmpty()) {
        binary.clear();
        TWebSocketFrame::OpCode opcode = frames.first().opCode();

        while (!frames.isEmpty()) {
            TWebSocketFrame frm = frames.takeFirst();
            binary += frm.payload();
            if (frm.isFinalFrame() && frm.state() == TWebSocketFrame::Completed) {
                break;
            }
        }

        // Starts worker thread
        TWebSocketWorker *worker = new TWebSocketWorker(TWebSocketWorker::Receiving, this, reqHeader.path());
        worker->setPayload(opcode, binary);
        startWorker(worker);
    }
}


void TWebSocket::startWorkerForOpening(const TSession &session)
{
    TWebSocketWorker *worker = new TWebSocketWorker(TWebSocketWorker::Opening, this, reqHeader.path());
    worker->setSession(session);
    startWorker(worker);
}


void TWebSocket::startWorkerForClosing()
{
    if (!closing.load()) {
        TWebSocketWorker *worker = new TWebSocketWorker(TWebSocketWorker::Closing, this, reqHeader.path());
        startWorker(worker);
    }
}


void TWebSocket::startWorker(TWebSocketWorker *worker)
{
    worker->moveToThread(Tf::app()->thread());
    connect(worker, SIGNAL(finished()), this, SLOT(releaseWorker()));
    ++myWorkerCounter; // count-up
    worker->start();
}


void TWebSocket::releaseWorker()
{
    TWebSocketWorker *worker = qobject_cast<TWebSocketWorker *>(sender());
    if (worker) {
        worker->deleteLater();
        --myWorkerCounter;  // count-down

        if (deleting.load()) {
            deleteLater();
        }
    }
}


void TWebSocket::deleteLater()
{
    tSystemDebug("TWebSocket::deleteLater  countWorkers:%d  deleting:%d", (int)myWorkerCounter, (bool)deleting);

    if (!deleting.exchange(true)) {
        startWorkerForClosing();
        return;
    }

    if ((int)myWorkerCounter == 0) {
        QTcpSocket::deleteLater();
    }
}


void TWebSocket::sendRawData(const QByteArray &data)
{
    if (data.isEmpty())
        return;

    qint64 total = 0;
    for (;;) {
        if (deleting.load())
            return;

        qint64 written = QTcpSocket::write(data.data() + total, qMin(data.length() - total, WRITE_LENGTH));
        if (Q_UNLIKELY(written <= 0)) {
            tWarn("socket write error: total:%d (%d)", (int)total, (int)written);
            break;
        }

        total += written;
        if (total >= data.length())
            break;

        if (Q_UNLIKELY(!waitForBytesWritten())) {
            tWarn("socket error: waitForBytesWritten function [%s]", qPrintable(errorString()));
            break;
        }
    }
}


qint64 TWebSocket::writeRawData(const QByteArray &data)
{
    // Calls send-function in main thread
    emit sendByWorker(data);
    return data.length();
}


void TWebSocket::disconnect()
{
    // Calls close-function in main thread
    emit disconnectByWorker();
    stopKeepAlive();
}


void TWebSocket::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == keepAliveTimer->timerId()) {
        sendPong();
    } else {
        QTcpSocket::timerEvent(event);
    }
}
