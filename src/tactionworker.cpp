/* Copyright (c) 2013-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tepollhttpsocket.h"
#include "tsystemglobal.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <TActionWorker>
#include <TAppSettings>
#include <THttpRequest>
#include <TWebApplication>
#include <TMultiplexingServer>
#include <atomic>


/*!
  \class TActionWorker
  \brief
*/


int64_t TActionWorker::writeResponse(THttpResponseHeader &header, QIODevice *body)
{
    if (keepAliveTimeout() > 0) {
        header.setRawHeader("Connection", "Keep-Alive");
    }
    accessLogger.setStatusCode(header.statusCode());

    // Check auto-remove
    bool autoRemove = false;
    QFile *f = dynamic_cast<QFile *>(body);
    if (f) {
        QString filePath = f->fileName();
        if (TActionContext::autoRemoveFiles.contains(filePath)) {
            TActionContext::autoRemoveFiles.removeAll(filePath);
            autoRemove = true;  // To remove after sent
        }
    }

    if (!TActionContext::stopped.load()) {
        _socket->sendData(header.toByteArray(), body, autoRemove, std::move(accessLogger));
    }
    accessLogger.close();
    return 0;
}


void TActionWorker::flushSocket()
{
    _socket->waitForDataSent(1000);
}


void TActionWorker::closeSocket()
{
    if (!TActionContext::stopped.load()) {
        _socket->disconnect();
    }
}


void TActionWorker::start(TEpollHttpSocket *sock)
{
    _socket = sock;
    _httpRequest += _socket->readRequest();
    _clientAddr = _socket->peerAddress();
    QList<THttpRequest> requests = THttpRequest::generate(_httpRequest, _clientAddr, this);

    // Loop for HTTP-pipeline requests
    for (THttpRequest &req : requests) {
        // Executes a action context
        TActionContext::execute(req);

        if (TActionContext::stopped.load()) {
            break;
        }
    }

    TActionContext::release();
    _httpRequest.clear();
    _clientAddr.clear();
    _socket = nullptr;
    // Deletes this
    //moveToThread(Tf::app()->databaseContextMainThread());
    //deleteLater();
}
