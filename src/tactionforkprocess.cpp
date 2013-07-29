/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <iostream>
#include <TActionForkProcess>
#include <TWebApplication>
#include <TSqlDatabasePool>
#include <TActionThread>
#include "thttpsocket.h"
#include "tsystemglobal.h"

/*!
  \class TActionForkProcess
  \brief The TActionForkProcess class provides a context of a
  forked process.
*/

TActionForkProcess *TActionForkProcess::currentActionContext = 0;


TActionForkProcess::TActionForkProcess(int socket)
    : QObject(), TActionContext(), httpSocket(0)
{
    TActionContext::socketDesc = socket;
}


TActionForkProcess::~TActionForkProcess()
{
    if (httpSocket)
        delete httpSocket;

    if (TActionContext::socketDesc > 0)
        TF_CLOSE(TActionContext::socketDesc);

    currentActionContext = 0;
}


void TActionForkProcess::emitError(int socketError)
{
    emit error(socketError);
}


TActionForkProcess *TActionForkProcess::currentContext()
{
    return currentActionContext;
}


void TActionForkProcess::start()
{
    if (currentActionContext)
        return;

    currentActionContext = this;
    std::cerr << "_accepted" << std::flush;  // send to tfmanager
    execute();

    // For cleanup
    QEventLoop eventLoop;
    while (eventLoop.processEvents()) {}

    emit finished();
    QCoreApplication::exit(1);
}


bool TActionForkProcess::readRequest()
{
    httpSocket = new THttpSocket;
    THttpRequest req;

    if (!httpSocket->setSocketDescriptor(TActionContext::socketDesc)) {
        emitError(httpSocket->error());
        goto socket_error;
    }
    TActionContext::socketDesc = 0;

    if (!TActionThread::readRequest(httpSocket, req)) {
        goto socket_error;
    }

    setHttpRequest(req);
    return true;

socket_error:
    delete httpSocket;
    httpSocket = 0;
    return false;
}


qint64 TActionForkProcess::writeResponse(THttpResponseHeader &header, QIODevice *body)
{
    header.setRawHeader("Connection", "close");
    return httpSocket->write(static_cast<THttpHeader*>(&header), body);
}


void TActionForkProcess::closeHttpSocket()
{
    httpSocket->close();
}


void TActionForkProcess::releaseHttpSocket()
{
    httpSocket->waitForBytesWritten();  // Flush socket
    httpSocket->close();  // disconnect

    // Destorys the object in the thread which created it
    delete httpSocket;
    httpSocket = 0;
}
