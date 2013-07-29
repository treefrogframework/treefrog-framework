/* Copyright (c) 013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QHostAddress>
#include <TWebApplication>
#include <THttpResponseHeader>
#include <THttpUtility>
#include "thttpsendbuffer.h"
#include "tsystemglobal.h"


THttpSendBuffer::THttpSendBuffer(const QByteArray &header, const QFileInfo &file, bool autoRemove, const TAccessLogger &logger)
    : arrayBuffer(header), bodyFile(0), fileRemove(autoRemove), accesslogger(logger), arraySentSize(0)
{
    if (file.exists() && file.isFile()) {
        bodyFile = new QFile(file.absoluteFilePath());
        if (!bodyFile->open(QIODevice::ReadOnly)) {
            tSystemWarn("file open failed: %s", qPrintable(file.absoluteFilePath()));
            release();
        }
    }
}


THttpSendBuffer::THttpSendBuffer(int statusCode, const QHostAddress &address, const QByteArray &method)
    : arrayBuffer(), bodyFile(0), fileRemove(false), accesslogger(), arraySentSize(0)
{
    accesslogger.open();
    accesslogger.setStatusCode(statusCode);
    accesslogger.setTimestamp(QDateTime::currentDateTime());
    accesslogger.setRemoteHost(address.toString().toLatin1());
    accesslogger.setRequest(method);

    THttpResponseHeader header;
    header.setStatusLine(statusCode, THttpUtility::getResponseReasonPhrase(statusCode));
    header.setRawHeader("Server", "TreeFrog server");
# if QT_VERSION >= 0x040700
    QDateTime utc = QDateTime::currentDateTimeUtc();
#else
    QDateTime utc = QDateTime::currentDateTime().toUTC();
#endif
    header.setRawHeader("Date", QLocale(QLocale::C).toString(utc, QLatin1String("ddd, dd MMM yyyy hh:mm:ss 'GMT'")).toLatin1());
    arrayBuffer += header.toByteArray();
}


THttpSendBuffer::~THttpSendBuffer()
{
    release();
}


void THttpSendBuffer::release()
{
    if (bodyFile) {
        if (fileRemove) {
            bodyFile->remove();
        }
        delete bodyFile;
        bodyFile = 0;
    }
}


int THttpSendBuffer::read(char *data, int maxSize)
{
    int ret = 0;
    int len;

    // Read the byte array
    len = qMin(arrayBuffer.length() - arraySentSize, maxSize);
    if (len > 0) {
        memcpy(data, arrayBuffer.constData() + arraySentSize, len);
        arraySentSize += len;
        data += len;
        ret += len;
        maxSize -= len;
    }

    // Read the file
    if (maxSize > 0 && !atEnd()) {
        len = bodyFile->read(data, maxSize);
        if (len < 0) {
            tSystemError("file read error: %s", qPrintable(bodyFile->fileName()));
            release();
        } else {
            ret += len;
        }
    }
    return ret;
}


bool THttpSendBuffer::atEnd() const
{
    return arraySentSize >= arrayBuffer.length()  && (!bodyFile || bodyFile->atEnd());
}
