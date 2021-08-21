/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "thttpsocket.h"
#include "tatomicptr.h"
#include "tfcore.h"
#include "tsystemglobal.h"
#include <QBuffer>
#include <QDir>
#include <TfCore>
#include <TAppSettings>
#include <TApplicationServerBase>
#include <THttpHeader>
#include <THttpResponse>
#include <TMultipartFormData>
#include <TTemporaryFile>
#include <chrono>
#include <ctime>
#include <thread>

constexpr uint READ_THRESHOLD_LENGTH = 4 * 1024 * 1024;  // bytes
constexpr qint64 WRITE_LENGTH = 1408;
constexpr int WRITE_BUFFER_LENGTH = WRITE_LENGTH * 512;

namespace {
TAtomicPtr<THttpSocket> socketManager[USHRT_MAX + 1];
std::atomic<ushort> point {0};
}

/*!
  \class THttpSocket
  \brief The THttpSocket class provides a socket for the HTTP.
*/

THttpSocket::THttpSocket(QByteArray &readBuffer, QObject *parent) :
    QObject(parent),
    _readBuffer(readBuffer)
{
    do {
        _sid = point.fetch_add(1);
    } while (!socketManager[_sid].compareExchange(nullptr, this));  // store a socket
    tSystemDebug("THttpSocket  sid:%d", _sid);

    connect(this, SIGNAL(requestWrite(const QByteArray &)), this, SLOT(writeRawData(const QByteArray &)), Qt::QueuedConnection);

    _idleElapsed = Tf::getMSecsSinceEpoch();
}


THttpSocket::~THttpSocket()
{
    abort();
    socketManager[_sid].compareExchangeStrong(this, nullptr);  // clear
    tSystemDebug("THttpSocket deleted  sid:%d", _sid);
}


QList<THttpRequest> THttpSocket::read()
{
    QList<THttpRequest> reqList;

    if (canReadRequest()) {
        if (_fileBuffer.isOpen()) {
            _fileBuffer.close();
            reqList << THttpRequest(_headerBuffer, _fileBuffer.fileName(), peerAddress());
            _headerBuffer.resize(0);
        } else {
            reqList = THttpRequest::generate(_readBuffer, peerAddress());
        }

        _lengthToRead = -1;
    }
    return reqList;
}


qint64 THttpSocket::write(const THttpHeader *header, QIODevice *body)
{
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
        QBuffer *buffer = dynamic_cast<QBuffer *>(body);
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


int THttpSocket::readRawData(char *data, int size, int msecs)
{
    if (Q_UNLIKELY(_socket <= 0)) {
        throw StandardException("Logic error", __FILE__, __LINE__);
    }

    int res = tf_poll_recv(_socket, msecs);
    if (res < 0) {
        tSystemError("socket poll error");
        abort();
        return -1;
    }

    if (!res) {
        // timeout
        return 0;
    }

    int len = tf_recv(_socket, data, size);
    if (len < 0) {
        abort();
        return -1;
    }

    if (len == 0) {
        tSystemDebug("Disconnected from remote host  [socket:%d]", _socket);
        abort();
        return 0;
    }

    _idleElapsed = Tf::getMSecsSinceEpoch();
    return len;
}


void THttpSocket::writeRawDataFromWebSocket(const QByteArray &data)
{
    emit requestWrite(data);
}


qint64 THttpSocket::writeRawData(const char *data, qint64 size)
{
    qint64 total = 0;

    if (Q_UNLIKELY(_socket <= 0)) {
        throw StandardException("Logic error", __FILE__, __LINE__);
    }

    if (Q_UNLIKELY(!data || size == 0)) {
        return total;
    }

    for (;;) {
        int res = tf_poll_send(_socket, 5000);
        if (res <= 0) {
            abort();
            break;
        } else {
            qint64 written = tf_send(_socket, data + total, qMin(size - total, WRITE_LENGTH));
            if (Q_UNLIKELY(written <= 0)) {
                tWarn("socket write error: total:%d (%d)", (int)total, (int)written);
                return -1;
            }

            total += written;
            if (total >= size) {
                break;
            }
        }
    }

    _idleElapsed = Tf::getMSecsSinceEpoch();
    return total;
}


qint64 THttpSocket::writeRawData(const QByteArray &data)
{
    return writeRawData(data.data(), data.size());
}


bool THttpSocket::waitForReadyReadRequest(int msecs)
{
    static const qint64 systemLimitBodyBytes = Tf::appSettings()->value(Tf::LimitRequestBody, "0").toLongLong() * 2;

    qint64 buflen = _readBuffer.capacity() - _readBuffer.size();
    qint64 len = readRawData(_readBuffer.data() + _readBuffer.size(), buflen, msecs);

    if (len < 0) {
        setSocketDescriptor(_socket, QAbstractSocket::ClosingState);
        return false;
    }

    if (len > 0) {
        _readBuffer.resize(_readBuffer.size() + len);

        if (_lengthToRead > 0) {
            // Writes to buffer
            if (_fileBuffer.isOpen()) {
                if (_fileBuffer.write(_readBuffer.data(), _readBuffer.size()) < 0) {
                    throw RuntimeException(QLatin1String("write error: ") + _fileBuffer.fileName(), __FILE__, __LINE__);
                }
                _lengthToRead = qMax(_lengthToRead - _readBuffer.size(), 0LL);
                _readBuffer.resize(0);
            } else {
                _lengthToRead = qMax(_lengthToRead - len, 0LL);
            }

        } else if (_lengthToRead < 0) {
            int idx = _readBuffer.indexOf(Tf::CRLFCRLF);
            if (idx > 0) {
                THttpRequestHeader header(_readBuffer);
                tSystemDebug("content-length: %lld", header.contentLength());

                if (Q_UNLIKELY(systemLimitBodyBytes > 0 && header.contentLength() > systemLimitBodyBytes)) {
                    throw ClientErrorException(Tf::RequestEntityTooLarge);  // Request Entity Too Large
                }

                _lengthToRead = qMax(idx + 4 + header.contentLength() - _readBuffer.length(), 0LL);

                if (header.contentLength() > READ_THRESHOLD_LENGTH || (header.contentLength() > 0 && header.contentType().trimmed().startsWith("multipart/form-data"))) {
                    _headerBuffer = _readBuffer.mid(0, idx + 4);
                    // Writes to file buffer
                    if (Q_UNLIKELY(!_fileBuffer.open())) {
                        throw RuntimeException(QLatin1String("temporary file open error: ") + _fileBuffer.fileTemplate(), __FILE__, __LINE__);
                    }
                    _fileBuffer.resize(0);  // truncate
                    if (_readBuffer.length() > idx + 4) {
                        tSystemDebug("fileBuffer name: %s", qUtf8Printable(_fileBuffer.fileName()));
                        if (_fileBuffer.write(_readBuffer.data() + idx + 4, _readBuffer.length() - (idx + 4)) < 0) {
                            throw RuntimeException(QLatin1String("write error: ") + _fileBuffer.fileName(), __FILE__, __LINE__);
                        }
                    }
                    _readBuffer.resize(0);
                } else {
                    if (_lengthToRead > 0) {
                        _readBuffer.reserve((idx + 4 + header.contentLength()) * 1.1);
                    }
                }
            } else {
                if (_readBuffer.size() > _readBuffer.capacity() * 0.8) {
                    _readBuffer.reserve(_readBuffer.capacity() * 2);
                }
            }
        } else {
            // do nothing
        }
    }
    return canReadRequest();
}


void THttpSocket::setSocketDescriptor(int socketDescriptor, QAbstractSocket::SocketState socketState)
{
    _socket = socketDescriptor;
    _state = socketState;

    if (_socket > 0) {
        auto peerInfo = TApplicationServerBase::getPeerInfo(_socket);
        _peerAddr = peerInfo.first;
        _peerPort = peerInfo.second;
    }
}


void THttpSocket::abort()
{
    if (_socket > 0) {
        tf_close_socket(_socket);
        tSystemDebug("Closed socket : %d", _socket);
        setSocketDescriptor(0, QAbstractSocket::ClosingState);
    } else {
        _state = QAbstractSocket::UnconnectedState;
    }
}


void THttpSocket::deleteLater()
{
    socketManager[_sid].compareExchange(this, nullptr);  // clear
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
    return (Tf::getMSecsSinceEpoch() - _idleElapsed) / 1000;
}

/*!
  Returns true if a HTTP request was received entirely; otherwise
  returns false.
  \fn bool THttpSocket::canReadRequest() const
*/
