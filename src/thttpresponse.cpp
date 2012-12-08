/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QFile>
#include <QBuffer>
#include <QLocale>
#include <THttpResponse>
#include <THttpUtility>
#include <TSystemGlobal>

/*!
  \class THttpResponse
  \brief The THttpResponse class contains response information for HTTP.
*/


THttpResponse::THttpResponse(const THttpResponseHeader &header, const QByteArray &body)
    : resHeader(header), tmpByteArray(body), bodyDevice(0)
{
    if (!tmpByteArray.isNull()) {
        bodyDevice = new QBuffer(&tmpByteArray);
    }
}


THttpResponse::~THttpResponse()
{
    if (bodyDevice)
        delete bodyDevice;
}


bool THttpResponse::isBodyNull() const
{
    return !bodyDevice;
}


void THttpResponse::setBody(const QByteArray &body)
{
    if (bodyDevice)
        delete bodyDevice;
    
    tmpByteArray = body;
    bodyDevice = (tmpByteArray.isNull()) ? 0 : new QBuffer(&tmpByteArray);
}


void THttpResponse::setBodyFile(const QString &filePath)
{
    if (bodyDevice) {
        delete bodyDevice;
        bodyDevice = 0;
    }

    QFile *fp = new QFile(filePath);
    if (fp->exists()) {
        if (fp->open(QIODevice::ReadOnly)) {
            // Success!
            bodyDevice = fp;
            return;
        } else {
            tSystemError("faild to open file: %s", qPrintable(filePath)); 
        }
    } else {
        tSystemError("file not found: %s", qPrintable(filePath));
    }
    
    // Error
    delete fp;
}
