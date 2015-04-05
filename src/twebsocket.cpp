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


TWebSocket::TWebSocket(QObject *parent)
    : QTcpSocket(parent), frames(), uuid(), reqHeader(), recvBuffer(),
      myWorkerCounter(0), deleting(false)
{
    uuid = QUuid::createUuid().toByteArray().replace('-', "");  // not thread safe
    uuid = uuid.mid(1, uuid.length() - 2);
    recvBuffer.reserve(BUFFER_RESERVE_SIZE);

    connect(this, SIGNAL(disconnected()), this, SLOT(deleteLater()));
    connect(this, SIGNAL(readyRead()), this, SLOT(readRequest()));
}


TWebSocket::~TWebSocket()
{ }


void TWebSocket::close()
{
    QTcpSocket::close();
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
        //close();
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
        TWebSocketWorker *worker = new TWebSocketWorker(this, reqHeader.path(), opcode, binary);
        worker->moveToThread(Tf::app()->thread());
        connect(worker, SIGNAL(finished()), this, SLOT(releaseWorker()));
        myWorkerCounter.fetchAndAddOrdered(1); // count-up
        worker->start();
    }
}


void TWebSocket::deleteLater()
{
    deleting = true;
    if (countWorkers() == 0) {
        QTcpSocket::deleteLater();
    }
}


void TWebSocket::releaseWorker()
{
    TWebSocketWorker *worker = dynamic_cast<TWebSocketWorker *>(sender());
    if (worker) {
        worker->deleteLater();
        myWorkerCounter.fetchAndAddOrdered(-1);  // count-down

        if (deleting) {
            deleteLater();
        }
    }
}


void TWebSocket::sendData(const QByteArray &data)
{
    if (data.isEmpty())
        return;

    qint64 total = 0;
    for (;;) {
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
