/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TActionWorker>
#include <THttpRequest>
#include <TMultiplexingServer>
#include "thttpsocket.h"
#include "tsystemglobal.h"

/*!
  \class TActionWorker
  \brief The TActionWorker class provides a thread context.
*/

TActionWorker::TActionWorker(int socket, const THttpRequest &request)
    : QThread(), TActionContext(socket)
{
    setHttpRequest(request);
}


TActionWorker::~TActionWorker()
{ }


qint64 TActionWorker::writeResponse(THttpResponseHeader &header, QIODevice *body)
{
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

    TMultiplexingServer::instance()->setSendRequest(socketDesc, static_cast<THttpHeader*>(&header), body, autoRemove, accessLogger);
    accessLogger.close();  // not write in this thread
    return 0;
}


void TActionWorker::closeHttpSocket()
{
    TMultiplexingServer::instance()->setDisconnectRequest(socketDesc);
}
