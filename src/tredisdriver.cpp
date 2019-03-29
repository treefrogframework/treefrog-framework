/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tredisdriver.h"
#include <QEventLoop>
#include <QElapsedTimer>
#include <QTcpSocket>
#include <TApplicationServerBase>
#include <TSystemGlobal>
#include <QThread>
using namespace Tf;

const int DEFAULT_PORT = 6379;
const int SEND_BUF_SIZE = 128 * 1024;
const int RECV_BUF_SIZE = 128 * 1024;


TRedisDriver::TRedisDriver() :
    TKvsDriver()
{
    _buffer.reserve(1023);
}


TRedisDriver::~TRedisDriver()
{
    close();
    delete _client;
}


bool TRedisDriver::isOpen() const
{
    return (_client) ? (_client->state() == QAbstractSocket::ConnectedState) : false;
}


bool TRedisDriver::open(const QString &, const QString &, const QString &, const QString &host, quint16 port, const QString &)
{
    if (isOpen()) {
        return true;
    }

    if (! _client) {
        _client = new QTcpSocket();
    }

    if (_client->state() != QAbstractSocket::UnconnectedState) {
        return false;
    }

#if QT_VERSION >= 0x050300
    // Sets socket options
    _client->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // Sets buffer size of socket
    int val = _client->socketOption(QAbstractSocket::SendBufferSizeSocketOption).toInt();
    if (val < SEND_BUF_SIZE) {
        _client->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, SEND_BUF_SIZE);
    }

    val = _client->socketOption(QAbstractSocket::ReceiveBufferSizeSocketOption).toInt();
    if (val < RECV_BUF_SIZE) {
        _client->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, RECV_BUF_SIZE);
    }
#endif

    _host = (host.isEmpty()) ? "localhost" : host;
    _port = (port == 0) ? DEFAULT_PORT : port;
    return connectToRedisServer();
}


bool TRedisDriver::connectToRedisServer()
{
    tSystemDebug("Redis open host:%s  port:%d", qPrintable(_host), _port);
    _client->connectToHost(_host, _port);

    bool ret = _client->waitForConnected(5000);
    if (Q_LIKELY(ret)) {
        tSystemDebug("Redis open successfully");
    } else {
        tSystemError("Redis open failed");
        close();
    }
    return ret;
}


void TRedisDriver::close()
{
    if (_client) {
        _client->close();
    }
}


bool TRedisDriver::readReply()
{
    if (Q_UNLIKELY(!isOpen())) {
        tSystemError("Not open Redis session  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    bool ret = _client->waitForReadyRead(5000);
    if (ret) {
        _buffer += _client->readAll();
    } else {
        tSystemWarn("Redis response timeout");
    }

    //tSystemDebug("#Redis response length: %d", _buffer.length());
    //tSystemDebug("#Redis response data: %s", _buffer.data());
    return ret;
}


bool TRedisDriver::request(const QList<QByteArray> &command, QVariantList &response)
{
    if (Q_UNLIKELY(!isOpen())) {
        tSystemError("Not open Redis session  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    bool ret = true;
    bool ok = false;
    QByteArray str;

    QByteArray cmd = toMultiBulk(command);
    tSystemDebug("Redis command: %s", cmd.data());
    _client->write(cmd);
    clearBuffer();

    for (;;) {
        if (! readReply()) {
            tSystemError("Redis read error   pos:%d  buflen:%d", _pos, _buffer.length());
            break;
        }

        switch (_buffer.at(_pos)) {
        case Error:
            ret = false;
            str = getLine(&ok);
            tSystemError("Redis error response: %s", qPrintable(str));
            break;

        case SimpleString:
            str = getLine(&ok);
            tSystemDebug("Redis response: %s", qPrintable(str));
            break;

        case Integer: {
            _pos++;
            int num = getNumber(&ok);
            if (ok) {
                response << num;
            }
            break; }

        case BulkString:
            str = parseBulkString(&ok);
            if (ok) {
                response << str;
            }
            break;

        case Array:
            response = parseArray(&ok);
            if (!ok) {
                response.clear();
            }
            break;

        default:
            tSystemError("Invalid protocol: %c  [%s:%d]", _buffer.at(_pos), __FILE__, __LINE__);
            ret = false;
            clearBuffer();
            goto parse_done;
            break;
        }

        if (ok) {
            if (_pos < _buffer.length()) {
                tSystemWarn("!!#### pos : %d  buf:%d", _pos, _buffer.length());
            }
            clearBuffer();
            break;
        }

        _pos = 0;
        // retry to read..
    }

parse_done:
    return ret;
}


QByteArray TRedisDriver::getLine(bool *ok)
{
    int idx = _buffer.indexOf(CRLF, _pos);
    if (idx < 0) {
        *ok = false;
        return QByteArray();
    }

    QByteArray ret = _buffer.mid(_pos, idx);
    _pos = idx + 2;
    *ok = true;
    return ret;
}


QByteArray TRedisDriver::parseBulkString(bool *ok)
{
    QByteArray str;
    int startpos = _pos;

    Q_ASSERT((int)_buffer[_pos] == BulkString);
    _pos++;

    int len = getNumber(ok);
    if (*ok) {
        if (len < -1) {
            tSystemError("Invalid length: %d  [%s:%d]", len, __FILE__, __LINE__);
            *ok = false;
        } else if (len == -1) {
            // null string
            tSystemDebug("Null string parsed");
        } else {
            if (_pos + 2 <= _buffer.length()) {
                str = (len > 0) ? _buffer.mid(_pos, len) : QByteArray("");
                _pos += len + 2;
            } else {
                *ok = false;
            }
        }
    }

    if (! *ok) {
        _pos = startpos;
    }
    return str;
}


QVariantList TRedisDriver::parseArray(bool *ok)
{
    QVariantList lst;
    int startpos = _pos;
    *ok = false;

    Q_ASSERT((int)_buffer[_pos] == Array);
    _pos++;

    int count = getNumber(ok);
    while (*ok) {
        switch (_buffer[_pos]) {
        case BulkString: {
            auto str = parseBulkString(ok);
            if (*ok) {
                lst << str;
            }
            break; }

        case Integer: {
            _pos++;
            int num = getNumber(ok);
            if (*ok) {
                lst << num;
            }
            break; }

        case Array: {
            auto var = parseArray(ok);
            if (*ok) {
                lst << QVariant(var);
            }
            break; }

        default:
            tSystemError("Bad logic  [%s:%d]", __FILE__, __LINE__);
            *ok = false;
            break;
        }

        if (lst.count() >= count) {
            break;
        }
    }

    if (! *ok) {
        _pos = startpos;
    }
    return lst;
}


int TRedisDriver::getNumber(bool *ok)
{
    int idx = _buffer.indexOf(CRLF, _pos);
    if (idx < 0) {
        *ok = false;
        return 0;
    }

    int num = _buffer.mid(_pos, idx - _pos).toInt();
    _pos = idx + 2;
    *ok = true;
    tSystemDebug("getNumber: %d", num);
    return num;
}


void TRedisDriver::clearBuffer()
{
    _buffer.resize(0);
    _pos = 0;
}


QByteArray TRedisDriver::toBulk(const QByteArray &data)
{
    QByteArray bulk("$");
    bulk += QByteArray::number(data.length());
    bulk += CRLF;
    bulk += data;
    bulk += CRLF;
    return bulk;
}


QByteArray TRedisDriver::toMultiBulk(const QList<QByteArray> &data)
{
    QByteArray mbulk("*");
    mbulk += QByteArray::number(data.count());
    mbulk += CRLF;
    for (auto &d : data) {
        mbulk += toBulk(d);
    }
    return mbulk;
}


void TRedisDriver::moveToThread(QThread *thread)
{
    if (!_client || _client->thread() == thread) {
        return;
    }

    int socket = 0;
    QAbstractSocket::SocketState state = QAbstractSocket::ConnectedState;

    if (_client->socketDescriptor() > 0) {
        socket = TApplicationServerBase::duplicateSocket(_client->socketDescriptor());
        state = _client->state();
        delete _client;
        _client = new QTcpSocket();
    }

    if (socket > 0) {
        _client->setSocketDescriptor(socket, state);
    }
    _client->moveToThread(thread);

}
