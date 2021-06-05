/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsendbuffer.h"
#include "tsystemglobal.h"
#include <TfCore>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QLocale>
#include <THttpResponseHeader>
#include <THttpUtility>
#include <TWebApplication>


TSendBuffer::TSendBuffer(const QByteArray &header, const QFileInfo &file, bool autoRemove, const TAccessLogger &logger) :
    arrayBuffer(header),
    fileRemove(autoRemove),
    accesslogger(logger)
{
    if (file.exists() && file.isFile()) {
        bodyFile = new QFile(file.absoluteFilePath());
        if (!bodyFile->open(QIODevice::ReadOnly)) {
            tSystemWarn("file open failed: %s", qUtf8Printable(file.absoluteFilePath()));
            release();
        }
    }
}


TSendBuffer::TSendBuffer(const QByteArray &header) :
    arrayBuffer(header)
{
}


TSendBuffer::TSendBuffer(int statusCode, const QHostAddress &address, const QByteArray &method)
{
    accesslogger.open();
    accesslogger.setStatusCode(statusCode);
    accesslogger.setTimestamp(QDateTime::currentDateTime());
    accesslogger.setRemoteHost(address.toString().toLatin1());
    accesslogger.setRequest(method);

    THttpResponseHeader header;
    header.setStatusLine(statusCode, THttpUtility::getResponseReasonPhrase(statusCode));
    header.setRawHeader("Server", "TreeFrog server");
    header.setCurrentDate();

    arrayBuffer += header.toByteArray();
}


TSendBuffer::~TSendBuffer()
{
    release();
}


void TSendBuffer::release()
{
    if (bodyFile) {
        if (fileRemove) {
            bodyFile->remove();
        }
        delete bodyFile;
        bodyFile = nullptr;
    }
}


void *TSendBuffer::getData(int &size)
{
    if (Q_UNLIKELY(size <= 0)) {
        tSystemError("Invalid data size. [%s:%d]", __FILE__, __LINE__);
        return nullptr;
    }

    if (startPos < arrayBuffer.length()) {
        size = qMin(arrayBuffer.length() - startPos, size);
        return arrayBuffer.data() + startPos;
    }

    if (!bodyFile || bodyFile->atEnd()) {
        size = 0;
        return nullptr;
    }

    arrayBuffer.reserve(size);
    size = bodyFile->read(arrayBuffer.data(), size);
    if (Q_UNLIKELY(size < 0)) {
        tSystemError("file read error: %s", qUtf8Printable(bodyFile->fileName()));
        size = 0;
        release();
        return nullptr;
    }

    arrayBuffer.resize(size);
    startPos = 0;
    return arrayBuffer.data();
}


bool TSendBuffer::seekData(int pos)
{
    if (Q_UNLIKELY(pos < 0)) {
        return false;
    }

    if (startPos + pos >= arrayBuffer.length()) {
        arrayBuffer.truncate(0);
        startPos = 0;
    } else {
        startPos += pos;
    }
    return true;
}


int TSendBuffer::prepend(const char *data, int maxSize)
{
    if (startPos > 0) {
        arrayBuffer.remove(0, startPos);
    }
    arrayBuffer.prepend(data, maxSize);
    startPos = 0;
    return maxSize;
}


bool TSendBuffer::atEnd() const
{
    return startPos >= arrayBuffer.length() && (!bodyFile || bodyFile->atEnd());
}
