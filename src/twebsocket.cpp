/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "twebsocket.h"
#include "tatomicptr.h"
#include "tdispatcher.h"
#include "turlroute.h"
#include "twebsocketworker.h"
#include <TWebApplication>
#include <QMap>
#include <QMutex>

constexpr int64_t WRITE_LENGTH = 1280;
constexpr int BUFFER_RESERVE_SIZE = 127;

namespace {
QMap<qintptr, TWebSocket*> socketManager;
QRecursiveMutex mutex;
}


TWebSocket::TWebSocket(int socketDescriptor, const QHostAddress &address, const THttpRequestHeader &header, QObject *parent) :
    QTcpSocket(parent),
    TAbstractWebSocket(header)
{
    setSocketDescriptor(socketDescriptor);
    setPeerAddress(address);

    recvBuffer.reserve(BUFFER_RESERVE_SIZE);
    QMutexLocker locker(&mutex);
    socketManager.insert(socketDescriptor, this);

    connect(this, SIGNAL(readyRead()), this, SLOT(readRequest()));
    connect(this, SIGNAL(sendByWorker(const QByteArray &)), this, SLOT(sendRawData(const QByteArray &)));
    connect(this, SIGNAL(disconnectByWorker()), this, SLOT(close()));
}


TWebSocket::~TWebSocket()
{
    tSystemDebug("~TWebSocket");
    QMutexLocker locker(&mutex);
    socketManager.remove(socketDescriptor());
}


void TWebSocket::close()
{
    QTcpSocket::close();
}


void TWebSocket::sendTextForPublish(const QString &text, const QObject *except)
{
    tSystemDebug("sendText  text len:{}  (pid:{})", (qint64)text.length(), (int)QCoreApplication::applicationPid());
    if (except != this) {
        TAbstractWebSocket::sendText(text);
    }
}


void TWebSocket::sendBinaryForPublish(const QByteArray &binary, const QObject *except)
{
    tSystemDebug("sendBinary  binary len:{}  (pid:{})", (qint64)binary.length(), (int)QCoreApplication::applicationPid());
    if (except != this) {
        TAbstractWebSocket::sendBinary(binary);
    }
}


void TWebSocket::sendPong(const QByteArray &data)
{
    tSystemDebug("sendPong  data len:{}  (pid:{})", (qint64)data.length(), (int)QCoreApplication::applicationPid());
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
    if (myWorkerCounter > 0) {
        tSystemWarn("Worker already running  (sd:{})", socketDescriptor());
        return;
    }

    int bytes = bytesAvailable();
    if (bytes > 0) {
        int sz = recvBuffer.size();
        recvBuffer.resize(sz + bytes);
        int rd = QTcpSocket::read(recvBuffer.data() + sz, bytes);
        if (Q_UNLIKELY(rd != bytes)) {
            tSystemError("socket read error");
            recvBuffer.resize(0);
            return;
        }
    }

    int len = parse(recvBuffer);
    if (len < 0) {
        tSystemError("WebSocket parse error [{}:{}]", __FILE__, __LINE__);
        disconnect();
        return;
    }

    QList<QPair<int, QByteArray>> payloads;
    QByteArray pay;

    while (canReadRequest()) {
        int opcode = frames.first().opCode();
        pay.resize(0);

        while (!frames.isEmpty()) {
            TWebSocketFrame frm = frames.takeFirst();
            pay += frm.payload();
            if (frm.isFinalFrame() && frm.state() == TWebSocketFrame::Completed) {
                payloads << qMakePair(opcode, pay);
                break;
            }
        }
    }

    if (!payloads.isEmpty()) {
        // Starts worker thread
        TWebSocketWorker *worker = new TWebSocketWorker(TWebSocketWorker::Receiving, this, reqHeader.path());
        worker->setPayloads(payloads);
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
    ++myWorkerCounter;  // count-up
    worker->start();
}


void TWebSocket::releaseWorker()
{
    TWebSocketWorker *worker = dynamic_cast<TWebSocketWorker *>(sender());
    if (worker) {
        worker->deleteLater();
        --myWorkerCounter;  // count-down

        if (deleting.load()) {
            deleteLater();
        } else {
            if (bytesAvailable() > 0) {
                readRequest();
            }
        }
    }
}


void TWebSocket::deleteLater()
{
    tSystemDebug("TWebSocket::deleteLater  countWorkers:{}  deleting:{}", (int)myWorkerCounter, (bool)deleting);

    if (!deleting.exchange(true)) {
        startWorkerForClosing();
        return;
    }

    if ((int)myWorkerCounter == 0) {
        QMutexLocker locker(&mutex);
        socketManager.remove(socketDescriptor());
        QTcpSocket::deleteLater();
    }
}


void TWebSocket::sendRawData(const QByteArray &data)
{
    if (data.isEmpty())
        return;

    int64_t total = 0;
    for (;;) {
        if (deleting.load()) {
            return;
        }

        if (QTcpSocket::bytesToWrite() > 0) {
            if (Q_UNLIKELY(!waitForBytesWritten())) {
                Tf::warn("websocket error: waitForBytesWritten function [{}]", errorString());
                break;
            }
        }

        int64_t written = QTcpSocket::write(data.data() + total, std::min((int64_t)data.length() - total, WRITE_LENGTH));
        if (Q_UNLIKELY(written <= 0)) {
            Tf::warn("websocket write error: total:{} ({})", (int)total, (int)written);
            break;
        }

        total += written;
        if (total >= data.length()) {
            break;
        }
    }
}


int64_t TWebSocket::writeRawData(const QByteArray &data)
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


TAbstractWebSocket *TWebSocket::searchSocket(qintptr socket)
{
    QMutexLocker locker(&mutex);
    return socketManager.value(socket, nullptr);
}


void TWebSocket::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == keepAliveTimer->timerId()) {
        sendPing();
    } else {
        QTcpSocket::timerEvent(event);
    }
}
