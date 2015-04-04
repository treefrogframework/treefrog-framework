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
      workerCounter(0), closing(false)
{
    if (!parent) {
        parent = Tf::app();
    }
    moveToThread(parent->thread());

    uuid = QUuid::createUuid().toByteArray().replace('-', "");  // not thread safe
    uuid = uuid.mid(1, uuid.length() - 2);
    recvBuffer.reserve(BUFFER_RESERVE_SIZE);

    connect(this, SIGNAL(disconnected()), this, SLOT(cleanup()));
    connect(this, SIGNAL(readyRead()), this, SLOT(readRequest()));
}


TWebSocket::~TWebSocket()
{ }


void TWebSocket::close()
{
    QTcpSocket::close();
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
        TWebSocketWorker *worker = new TWebSocketWorker(socketUuid(), reqHeader.path(), opcode, binary);
        worker->moveToThread(Tf::app()->thread());
        connect(worker, SIGNAL(finished()), worker, SLOT(cleanupWorker()));
        workerCounter.fetchAndAddOrdered(1);
        worker->start();
    }
}


void TWebSocket::cleanup()
{
    closing = true;
    if (countWorkers() == 0) {
        deleteLater();
    }
}


void TWebSocket::cleanupWorker()
{
    TWebSocketWorker *worker = dynamic_cast<TWebSocketWorker *>(sender());
    if (worker) {
        workerCounter.fetchAndAddOrdered(-1);
        worker->deleteLater();

        if (closing) {
            cleanup();
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
