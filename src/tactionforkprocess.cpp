/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TActionForkProcess>
#include <TWebApplication>
#include <TActionThread>
#include <iostream>
#include "tfcore.h"
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
        tf_close(TActionContext::socketDesc);

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

    QList<THttpRequest> reqs;
    QEventLoop eventLoop;
    httpSocket = new THttpSocket;

    if (!httpSocket->setSocketDescriptor(TActionContext::socketDesc)) {
        emitError(httpSocket->error());
        goto socket_error;
    }
    TActionContext::socketDesc = 0;

    for (;;) {
        reqs = TActionThread::readRequest(httpSocket);
        tSystemDebug("HTTP request count: %d", reqs.count());

        if (reqs.isEmpty()) {
            break;
        }

        for (QMutableListIterator<THttpRequest> it(reqs); it.hasNext(); ) {
            THttpRequest &req = it.next();
            TActionContext::execute(req, httpSocket->socketUuid());

            httpSocket->flush();  // Flush socket
            TActionContext::release();
        }

        if (!httpSocket->waitForReadyRead(5000))
            break;
    }

    closeHttpSocket();  // disconnect

    // For cleanup
    while (eventLoop.processEvents()) {}

    emit finished();
    QCoreApplication::exit(1);

socket_error:
    delete httpSocket;
    httpSocket = 0;
}


qint64 TActionForkProcess::writeResponse(THttpResponseHeader &header, QIODevice *body)
{
    header.setRawHeader("Connection", "Keep-Alive");
    return httpSocket->write(static_cast<THttpHeader*>(&header), body);
}


void TActionForkProcess::closeHttpSocket()
{
    httpSocket->close();
}

