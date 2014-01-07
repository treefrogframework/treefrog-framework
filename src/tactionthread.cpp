/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QCoreApplication>
#include <QEventLoop>
#include <QAtomicInt>
#include <TActionThread>
#include <THttpRequest>
#include <unistd.h>
#include "thttpsocket.h"
#include "tsystemglobal.h"

static QAtomicInt threadCounter;

int TActionThread::threadCount()
{
#if QT_VERSION >= 0x050000
    return threadCounter.load();
#else
    return (int)threadCounter;
#endif
}

bool TActionThread::waitForAllDone(int msec)
{
    int cnt;
    QTime time;
    time.start();

    while ((cnt = threadCount()) > 0) {
        if (time.elapsed() > msec) {
            break;
        }

        Tf::msleep(10);
        qApp->processEvents();
    }
    return cnt == 0;
}

/*!
  \class TActionThread
  \brief The TActionThread class provides a thread context.
*/

TActionThread::TActionThread(int socket)
    : QThread(), TActionContext(), httpSocket(0)
{
    threadCounter.fetchAndAddOrdered(1);
    TActionContext::socketDesc = socket;
}


TActionThread::~TActionThread()
{
    if (httpSocket)
        delete httpSocket;

    if (TActionContext::socketDesc > 0)
        TF_CLOSE(TActionContext::socketDesc);

    threadCounter.fetchAndAddOrdered(-1);
}


void TActionThread::run()
{
    QList<THttpRequest> reqs;
    QEventLoop eventLoop;
    httpSocket = new THttpSocket;

    if (!httpSocket->setSocketDescriptor(TActionContext::socketDesc)) {
        emitError(httpSocket->error());
        goto socket_error;
    }
    TActionContext::socketDesc = 0;

    for (;;) {
        reqs = readRequest(httpSocket);
        tSystemDebug("HTTP request count: %d", reqs.count());

        if (reqs.isEmpty())
            break;

        for (QMutableListIterator<THttpRequest> it(reqs); it.hasNext(); ) {
            THttpRequest &req = it.next();
            TActionContext::execute(req);

            httpSocket->flush();  // Flush socket
            TActionContext::release();
        }

        if (!httpSocket->waitForReadyRead(5000))
            break;
    }

    closeHttpSocket();  // disconnect

    // For cleanup
    while (eventLoop.processEvents()) {}

socket_error:
    delete httpSocket;
    httpSocket = 0;
}


void TActionThread::emitError(int socketError)
{
    emit error(socketError);
}


QList<THttpRequest> TActionThread::readRequest(THttpSocket *socket)
{
    QList<THttpRequest> reqs;
    while (!socket->canReadRequest()) {
        // Check idle timeout
        if (socket->idleTime() >= 10) {
            tSystemWarn("Reading a socket timed out after 10 seconds. Descriptor:%d", (int)socket->socketDescriptor());
            break;
        }

        if (socket->socketDescriptor() <= 0) {
            tSystemWarn("Invalid descriptor (disconnected) : %d", (int)socket->socketDescriptor());
            break;
        }

        socket->waitForReadyRead(10);
    }

    if (!socket->canReadRequest()) {
        socket->abort();
    } else {
        reqs = socket->read();
    }

    return reqs;
}


qint64 TActionThread::writeResponse(THttpResponseHeader &header, QIODevice *body)
{
    header.setRawHeader("Connection", "Keep-Alive");
    return httpSocket->write(static_cast<THttpHeader*>(&header), body);
}


void TActionThread::closeHttpSocket()
{
    httpSocket->close();
}
