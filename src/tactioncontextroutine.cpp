/* Copyright (c) 2025, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tactioncontextroutine.h"
#include "THttpRequest"
#include <QMutexLocker>


void TActionContextRoutine::start(QByteArray &readBuffer)
{
    TActionContext::setCurrentActionContext(this);

    // TODO TODO: Seach address
    THttpRequest request = THttpRequest::generate(readBuffer, QHostAddress("localhost"), this);
    execute(request);

    TActionContext::setCurrentActionContext(nullptr);
}


int64_t TActionContextRoutine::writeResponse(THttpResponseHeader &header, QIODevice *body)
{
   if (keepAliveTimeout() > 0) {
        header.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("Keep-Alive"));
    }
    // Writes HTTP header
    result.response = header.toByteArray();

    if (body) {
        if (auto *buf = dynamic_cast<QBuffer*>(body); buf) {  // dynamic_cast is faster for QBuffer
            result.response += buf->buffer();
        } else if (auto *file = qobject_cast<QFile*>(body); file) {
            result.fileName = file->fileName();
        } else {
            tSystemError("Invalid body [{}:{}]", __FILE__, __LINE__);
        }
    }
    return 0;
}
