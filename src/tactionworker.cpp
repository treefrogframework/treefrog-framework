/* Copyright (c) 2013-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TActionWorker>
#include <THttpRequest>
#include <TAppSettings>
#include <TMultiplexingServer>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <atomic>
#include "tepollhttpsocket.h"
#include "tsystemglobal.h"


/*!
  \class TActionWorker
  \brief
*/


TActionWorker *TActionWorker::instance()
{
    static TActionWorker globalInstance;
    return &globalInstance;
}


qint64 TActionWorker::writeResponse(THttpResponseHeader &header, QIODevice *body)
{
    static int keepAliveTimeout = []() {
        int timeout = Tf::appSettings()->value(Tf::HttpKeepAliveTimeout, "10").toInt();
        return qMax(timeout, 0);
    }();

    if (keepAliveTimeout > 0) {
        header.setRawHeader("Connection", "Keep-Alive");
    }
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

    if (!TActionContext::stopped.load()) {
        socket->sendData(header.toByteArray(), body, autoRemove, accessLogger);
    }
    accessLogger.close();  // not write in this thread
    return 0;
}


void TActionWorker::closeHttpSocket()
{
    if (!TActionContext::stopped.load()) {
        socket->disconnect();
    }
}


void TActionWorker::start(TEpollHttpSocket *sock)
{
    socket = sock;
    httpRequest = socket->readRequest();
    clientAddr = socket->peerAddress().toString();
    QList<THttpRequest> reqs = THttpRequest::generate(httpRequest, QHostAddress(clientAddr));

    // Loop for HTTP-pipeline requests
    for (QMutableListIterator<THttpRequest> it(reqs); it.hasNext(); ) {
        THttpRequest &req = it.next();

        // Executes a action context
        TActionContext::execute(req, socket->socketId());

        if (TActionContext::stopped.load()) {
            break;
        }
    }

    TActionContext::release();
    httpRequest.clear();
    clientAddr.clear();
}
