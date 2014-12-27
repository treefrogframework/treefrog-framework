/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QTimer>
#include <QDir>
#include <QBuffer>
#include <TTemporaryFile>
#include <TAppSettings>
#include <THttpResponse>
#include <THttpHeader>
#include <TMultipartFormData>
#include "thttpsocket.h"
#include "tsystemglobal.h"

const uint   READ_THRESHOLD_LENGTH = 2 * 1024 * 1024; // bytes
const qint64 WRITE_LENGTH = 1280;
const int    WRITE_BUFFER_LENGTH = WRITE_LENGTH * 512;

/*!
  \class THttpSocket
  \brief The THttpSocket class provides a socket for the HTTP.
*/

THttpSocket::THttpSocket(QObject *parent)
    : QTcpSocket(parent), lengthToRead(-1), lastProcessed(Tf::currentDateTimeSec())
{
    T_TRACEFUNC("");
    connect(this, SIGNAL(readyRead()), this, SLOT(readRequest()));
}


THttpSocket::~THttpSocket()
{
    T_TRACEFUNC("");
}


QList<THttpRequest> THttpSocket::read()
{
    T_TRACEFUNC("");

    QList<THttpRequest> reqList;

    if (canReadRequest()) {
        if (fileBuffer.isOpen()) {
            fileBuffer.close();
            THttpRequest req(readBuffer, fileBuffer.fileName(), peerAddress());
            reqList << req;
        } else {
            reqList = THttpRequest::generate(readBuffer, peerAddress());
        }
        readBuffer.clear();
        lengthToRead = -1;
    }
    return reqList;
}


qint64 THttpSocket::write(const THttpHeader *header, QIODevice *body)
{
    T_TRACEFUNC("");

    if (body && !body->isOpen()) {
        if (!body->open(QIODevice::ReadOnly)) {
            tWarn("open failed");
            return -1;
        }
    }

    // Writes HTTP header
    QByteArray hdata = header->toByteArray();
    qint64 total = writeRawData(hdata.data(), hdata.size());
    if (total < 0) {
        return -1;
    }

    if (body) {
        QBuffer *buffer = qobject_cast<QBuffer *>(body);
        if (buffer) {
            if (writeRawData(buffer->data().data(), buffer->size()) != buffer->size()) {
                return -1;
            }
            total += buffer->size();
        } else {
            QByteArray buf(WRITE_BUFFER_LENGTH, 0);
            qint64 readLen = 0;
            while ((readLen = body->read(buf.data(), buf.size())) > 0) {
                if (writeRawData(buf.data(), readLen) != readLen) {
                    return -1;
                }
                total += readLen;
            }
        }
    }
    return total;
}


qint64 THttpSocket::writeRawData(const char *data, qint64 size)
{
    qint64 total = 0;
    for (;;) {
        qint64 written = QTcpSocket::write(data + total, qMin(size - total, WRITE_LENGTH));
        if (written <= 0) {
            tWarn("socket write error: total:%d (%d)", (int)total, (int)written);
            return -1;
        }
        total += written;
        if (total >= size)
            break;

        if (!waitForBytesWritten()) {
            tWarn("socket error: waitForBytesWritten function [%s]", qPrintable(errorString()));
            break;
        }
    }
    return total;
}

/*!
  Returns true if a HTTP request was received entirely; otherwise
  returns false.
*/
bool THttpSocket::canReadRequest() const
{
    T_TRACEFUNC("");
    return (lengthToRead == 0);
}


void THttpSocket::readRequest()
{
    T_TRACEFUNC("");
    uint limitBodyBytes = Tf::appSettings()->value(Tf::LimitRequestBody, "0").toUInt();
    qint64 bytes = 0;
    QByteArray buf;

    while ((bytes = bytesAvailable()) > 0) {
        buf.resize(bytes);
        bytes = QTcpSocket::read(buf.data(), bytes);
        if (bytes < 0) {
            tSystemError("socket read error");
            break;
        }
        lastProcessed = Tf::currentDateTimeSec();

        if (lengthToRead > 0) {
            // Writes to buffer
            if (fileBuffer.isOpen()) {
                if (fileBuffer.write(buf.data(), bytes) < 0) {
                    throw RuntimeException(QLatin1String("write error: ") + fileBuffer.fileName(), __FILE__, __LINE__);
                }
            } else {
                readBuffer.append(buf.data(), bytes);
            }
            lengthToRead = qMax(lengthToRead - bytes, 0LL);

        } else if (lengthToRead < 0) {
            readBuffer.append(buf);
            int idx = readBuffer.indexOf("\r\n\r\n");
            if (idx > 0) {
                THttpRequestHeader header(readBuffer);
                tSystemDebug("content-length: %d", header.contentLength());

                if (limitBodyBytes > 0 && header.contentLength() > limitBodyBytes) {
                    throw ClientErrorException(413);  // Request Entity Too Large
                }

                lengthToRead = qMax(idx + 4 + (qint64)header.contentLength() - readBuffer.length(), 0LL);

                if (header.contentType().trimmed().startsWith("multipart/form-data")
                    || header.contentLength() > READ_THRESHOLD_LENGTH) {
                    // Writes to file buffer
                    if (!fileBuffer.open()) {
                        throw RuntimeException(QLatin1String("temporary file open error: ") + fileBuffer.fileTemplate(), __FILE__, __LINE__);
                    }
                    if (readBuffer.length() > idx + 4) {
                        tSystemDebug("fileBuffer name: %s", qPrintable(fileBuffer.fileName()));
                        if (fileBuffer.write(readBuffer.data() + idx + 4, readBuffer.length() - (idx + 4)) < 0) {
                            throw RuntimeException(QLatin1String("write error: ") + fileBuffer.fileName(), __FILE__, __LINE__);
                        }
                    }
                }
            }
        } else {
            // do nothing
            break;
        }

        if (lengthToRead == 0) {
            emit newRequest();
        }
    }
}

/*!
  Returns the number of seconds of idle time.
*/
int THttpSocket::idleTime() const
{
    return lastProcessed.secsTo(Tf::currentDateTimeSec());
}
