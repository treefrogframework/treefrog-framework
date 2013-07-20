/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QEventLoop>
#include <TActionThread>
#include <THttpRequest>
#include "thttpsocket.h"
#include "tsystemglobal.h"

/*!
  \class TActionThread
  \brief The TActionThread class provides a thread context.
*/


TActionThread::TActionThread(int socket)
    : QThread(), TActionContext(socket), httpSocket(0)
{ }


TActionThread::~TActionThread()
{
    if (httpSocket)
        delete httpSocket;
 }


void TActionThread::run()
{
    execute();

    // For cleanup
    QEventLoop eventLoop;
    while (eventLoop.processEvents()) {}
}


void TActionThread::emitError(int socketError)
{
    emit error(socketError);
}


bool TActionThread::readRequest(THttpSocket *socket, THttpRequest &request)
{
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
        return false;
    }

    request = socket->read();
    return true;
}


bool TActionThread::readRequest()
{
    httpSocket = new THttpSocket;
    THttpRequest req;

    if (!httpSocket->setSocketDescriptor(TActionContext::socketDesc)) {
        emitError(httpSocket->error());
        goto socket_error;
    }
    TActionContext::socketDesc = 0;

    if (!readRequest(httpSocket, req)) {
        goto socket_error;
    }

    setHttpRequest(req);
    return true;

socket_error:
    delete httpSocket;
    httpSocket = 0;
    return false;
}


qint64 TActionThread::writeResponse(THttpResponseHeader &header, QIODevice *body)
{
    header.setRawHeader("Connection", "close");
    return httpSocket->write(static_cast<THttpHeader*>(&header), body);
}


void TActionThread::closeHttpSocket()
{
    httpSocket->close();
}


void TActionThread::releaseHttpSocket()
{
    TActionContext::accessLog.timestamp = QDateTime::currentDateTime();
    writeAccessLog(TActionContext::accessLog);  // Writes access log

    httpSocket->waitForBytesWritten();  // Flush socket
    httpSocket->close();  // disconnect

    // Destorys the object in the thread which created it
    delete httpSocket;
    httpSocket = 0;
}
