/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TActionWorker>
#include <THttpRequest>
#include <TMultiplexingServer>
#include <QCoreApplication>
#include <QAtomicInt>
#include "thttpsocket.h"
#include "tepollhttpsocket.h"
#include "tsystemglobal.h"

// Counter of action workers  (Note: workerCount != contextCount)
QAtomicInt workerCounter;


int TActionWorker::workerCount()
{
#if QT_VERSION >= 0x050000
    return workerCounter.load();
#else
    return (int)workerCounter;
#endif
}


bool TActionWorker::waitForAllDone(int msec)
{
    int cnt;
    QTime time;
    time.start();

    while ((cnt = workerCount()) > 0) {
        if (time.elapsed() > msec) {
            break;
        }

        Tf::msleep(10);
        qApp->processEvents();
    }
    return cnt == 0;
}

/*!
  \class TActionWorker
  \brief The TActionWorker class provides a thread context.
*/

TActionWorker::TActionWorker(TEpollHttpSocket *socket, QObject *parent)
    : QThread(parent), TActionContext(), httpRequest(), clientAddr(), socketId(socket->id())
{
    workerCounter.fetchAndAddOrdered(1);
    httpRequest = socket->recvBuf.read(INT_MAX);
    clientAddr = socket->clientAddress().toString();
}


TActionWorker::~TActionWorker()
{
    tSystemDebug("TActionWorker::~TActionWorker");
    workerCounter.fetchAndAddOrdered(-1);
}


qint64 TActionWorker::writeResponse(THttpResponseHeader &header, QIODevice *body)
{
    header.setRawHeader("Connection", "Keep-Alive");
    accessLogger.setStatusCode(header.statusCode());

    // Check auto-remove
    bool autoRemove = false;
    QFile *f = qobject_cast<QFile *>(body);
    if (f) {
        QString filePath = f->fileName();
        if (TActionContext::autoRemoveFiles.contains(filePath)) {
            TActionContext::autoRemoveFiles.removeAll(filePath);
            autoRemove = true;  // To remove after sent
        }
    }

    if (!TActionContext::stopped) {
        TEpollSocket::setSendData(socketId, header.toByteArray(), body, autoRemove, accessLogger);
    }
    accessLogger.close();  // not write in this thread
    return 0;
}


void TActionWorker::closeHttpSocket()
{
    if (!TActionContext::stopped) {
        TEpollSocket::setDisconnect(socketId);
    }
}


void TActionWorker::run()
{
    QList<THttpRequest> reqs = THttpRequest::generate(httpRequest, QHostAddress(clientAddr));

    // Loop for HTTP-pipeline requests
    for (QMutableListIterator<THttpRequest> it(reqs); it.hasNext(); ) {
        THttpRequest &req = it.next();

        // Executes a action context
        TActionContext::execute(req);
        TActionContext::release();

        if (TActionContext::stopped) {
            break;
        }
    }

    httpRequest.clear();
    clientAddr.clear();
}
