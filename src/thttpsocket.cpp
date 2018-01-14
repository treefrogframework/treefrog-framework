/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <ctime>
#include <QTimer>
#include <QDir>
#include <QBuffer>
#include <TTemporaryFile>
#include <TAppSettings>
#include <THttpResponse>
#include <THttpHeader>
#include <TMultipartFormData>
#include "thttpsocket.h"
#include "tatomicptr.h"
#include "tsystemglobal.h"

const uint   READ_THRESHOLD_LENGTH = 2 * 1024 * 1024; // bytes
const qint64 WRITE_LENGTH = 1408;
const int    WRITE_BUFFER_LENGTH = WRITE_LENGTH * 512;
const int    SEND_BUF_SIZE = 16 * 1024;
const int    RECV_BUF_SIZE = 128 * 1024;

static TAtomicPtr<THttpSocket> socketManager[USHRT_MAX + 1];
static std::atomic<ushort> point {0};

/*!
  \class THttpSocket
  \brief The THttpSocket class provides a socket for the HTTP.
*/

THttpSocket::THttpSocket(QObject *parent)
    : QTcpSocket(parent), lengthToRead(-1)
{
    T_TRACEFUNC("");

    do {
        sid = point.fetch_add(1);
    } while (!socketManager[sid].compareExchange(nullptr, this)); // store a socket
    tSystemDebug("THttpSocket  sid:%d", sid);

    connect(this, SIGNAL(readyRead()), this, SLOT(readRequest()));
    connect(this, SIGNAL(requestWrite(const QByteArray&)), this, SLOT(writeRawData(const QByteArray&)), Qt::QueuedConnection);

    idleElapsed = std::time(nullptr);
}


THttpSocket::~THttpSocket()
{
    T_TRACEFUNC("");
    socketManager[sid].compareExchangeStrong(this, nullptr); // clear
    tSystemDebug("THttpSocket deleted  sid:%d", sid);
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
            fileBuffer.resize(0);
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


void THttpSocket::writeRawDataFromWebSocket(const QByteArray &data)
{
    emit requestWrite(data);
}


qint64 THttpSocket::writeRawData(const char *data, qint64 size)
{
    qint64 total = 0;

    if (Q_UNLIKELY(!data || size == 0)) {
        return total;
    }

    for (;;) {
        if (QTcpSocket::bytesToWrite() > SEND_BUF_SIZE * 3 / 4) {
            if (Q_UNLIKELY(!waitForBytesWritten())) {
                tWarn("socket error: waitForBytesWritten function [%s]", qPrintable(errorString()));
                break;
            }
        }

        qint64 written = QTcpSocket::write(data + total, qMin(size - total, WRITE_LENGTH));
        if (Q_UNLIKELY(written <= 0)) {
            tWarn("socket write error: total:%d (%d)", (int)total, (int)written);
            return -1;
        }

        total += written;
        if (total >= size) {
            break;
        }
    }

    idleElapsed = std::time(nullptr);
    return total;
}


qint64 THttpSocket::writeRawData(const QByteArray &data)
{
    return writeRawData(data.data(), data.size());
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
        int rd = QTcpSocket::read(buf.data(), bytes);
        if (Q_UNLIKELY(rd != bytes)) {
            tSystemError("socket read error");
            buf.resize(0);
            break;
        }
        idleElapsed = std::time(nullptr);

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

                if (Q_UNLIKELY(limitBodyBytes > 0 && header.contentLength() > limitBodyBytes)) {
                    throw ClientErrorException(413);  // Request Entity Too Large
                }

                lengthToRead = qMax(idx + 4 + (qint64)header.contentLength() - readBuffer.length(), 0LL);

                if (header.contentType().trimmed().startsWith("multipart/form-data")
                    || header.contentLength() > READ_THRESHOLD_LENGTH) {
                    // Writes to file buffer
                    if (Q_UNLIKELY(!fileBuffer.open())) {
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


bool THttpSocket::setSocketDescriptor(qintptr socketDescriptor, SocketState socketState, OpenMode openMode)
{
    bool ret  = QTcpSocket::setSocketDescriptor(socketDescriptor, socketState, openMode);
    if (ret) {
        // Sets socket options
        QTcpSocket::setSocketOption(QAbstractSocket::LowDelayOption, 1);

        // Sets buffer size of socket
#if QT_VERSION >= 0x050300
        int val = QTcpSocket::socketOption(QAbstractSocket::SendBufferSizeSocketOption).toInt();
        if (val < SEND_BUF_SIZE) {
            QTcpSocket::setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, SEND_BUF_SIZE);
        }

        val = QTcpSocket::socketOption(QAbstractSocket::ReceiveBufferSizeSocketOption).toInt();
        if (val < RECV_BUF_SIZE) {
            QTcpSocket::setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, RECV_BUF_SIZE);
        }
#else
# ifdef Q_OS_UNIX
        int res, bufsize;

        bufsize = SEND_BUF_SIZE;
        res = setsockopt((int)socketDescriptor, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
        if (res < 0) {
            tSystemWarn("setsockopt error [SO_SNDBUF] fd:%d", (int)socketDescriptor);
        }

        bufsize = RECV_BUF_SIZE;
        res = setsockopt((int)socketDescriptor, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
        if (res < 0) {
            tSystemWarn("setsockopt error [SO_RCVBUF] fd:%d", (int)socketDescriptor);
        }
# endif
#endif
    }
    return ret;
}


void THttpSocket::deleteLater()
{
    socketManager[sid].compareExchange(this, nullptr); // clear
    QObject::deleteLater();
}


THttpSocket *THttpSocket::searchSocket(int sid)
{
    return socketManager[sid & 0xffff].load();
}

/*!
  Returns the number of seconds of idle time.
*/
int THttpSocket::idleTime() const
{
    return (uint)std::time(nullptr) - idleElapsed;
}
