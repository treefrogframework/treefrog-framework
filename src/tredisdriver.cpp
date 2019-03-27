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
#include <thread>
using namespace Tf;

const int DEFAULT_PORT = 6379;


TRedisDriver::TRedisDriver() :
    TKvsDriver(),
    _client(new QTcpSocket())
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
    return _client->state() == QAbstractSocket::ConnectedState;
}


bool TRedisDriver::open(const QString &, const QString &, const QString &, const QString &host, quint16 port, const QString &)
{
    if (isOpen()) {
        return true;
    }

    if (_client->state() != QAbstractSocket::UnconnectedState) {
        return false;
    }

    QString hst = (host.isEmpty()) ? "localhost" : host;

    if (port <= 0) {
        port = DEFAULT_PORT;
    }

    tSystemDebug("Redis open host:%s  port:%d", qPrintable(hst), port);
    _client->connectToHost(hst, port);

    bool ret = _client->waitForConnected(5000);
    if (Q_LIKELY(ret)) {
        tSystemDebug("Redis open successfully");
    } else {
        tSystemError("Redis open failed");
        close();
    }
    return ret;

    // // function waitForConnected()
    // auto waitForConnected = [=]() {
    //     bool ret = waitForState(QAbstractSocket::ConnectedState, 5000);
    //     if (Q_LIKELY(ret)) {
    //         tSystemDebug("Redis open successfully");
    //     } else {
    //         tSystemError("Redis open failed");
    //         close();
    //     }
    //     return ret;
    // };

    // // QObject::connect(_client, &QTcpSocket::disconnected, []() {
    // //     tSystemError("Redis disconnected");
    // //     _client->connectToHost(hst, port);
    // //     waitForConnected();
    // // });

    // return waitForConnected();
}


void TRedisDriver::close()
{
    _client->close();
}


bool TRedisDriver::readReply()
{
    if (Q_UNLIKELY(!isOpen())) {
        tSystemError("Not open Redis session  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

/*
    if (Q_UNLIKELY(_pos > 0)) {
        tSystemWarn("remain buffer: %d", _pos);
        _buffer.remove(0, _pos);
    //    _buffer.resize(0);
        _pos = 0;
    }
*/

    // QEventLoop eventLoop;
    // QElapsedTimer timer;
    // timer.start();

    // int startlen = _buffer.length();
    // tSystemWarn("## startlen: %d", startlen);

    bool ret = _client->waitForReadyRead(5000);
    if (ret) {
        _buffer += _client->readAll();
        // if (_buffer.length() > startlen) {
        //     break;
        // }

        // if (timer.elapsed() >= 10000) {
        //     tSystemWarn("Read timeout");
        //     break;
        // }

        //std::this_thread::yield();  // context switch
        //Tf::msleep(5);
        //tSystemWarn("## evetloop");
        //while (eventLoop.processEvents()) {}
    }

    tSystemDebug("#Redis response length: %d", _buffer.length());
    tSystemDebug("#Redis response data: %s", _buffer.data());
    return ret;
}


bool TRedisDriver::request(const QList<QByteArray> &command, QVariantList &response)
{
    if (Q_UNLIKELY(!isOpen())) {
        tSystemError("Not open Redis session  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    bool ret = true;
    QByteArray str;
    bool ok = false;
    int startpos = _pos;

    QByteArray cmd = toMultiBulk(command);
    tSystemDebug("Redis command: %s", cmd.data());
    _client->write(cmd);
    //_client->waitForBytesWritten();
    //_client->flush();
    //clearBuffer();

    for (;;) {
        if (! readReply()) {
            clearBuffer();
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
            _buffer.remove(0, _pos);
            _pos = _buffer.length();
            break;
        } else {
            _pos = startpos;
            tSystemWarn("## repeat!!!");
        }
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

#if 0
    int num = 0;
    int c = 1;
    char d = _buffer[_pos++];

    if (d == '-') {
        c = -1;
        d = _buffer[_pos++];
    }

    while (d >= '0' && d <= '9') {
        num *= 10;
        num += d - '0';
        d = _buffer[_pos++];
    }

    _pos = idx + 2;
    *ok = true;
    return num * c;
#else
    int num = _buffer.mid(_pos, idx - _pos).toInt();
    _pos = idx + 2;
    *ok = true;
    tSystemDebug("getNumber: %d", num);
    return num;
#endif
}


void TRedisDriver::clearBuffer()
{
    if (_pos > 0) {
        tSystemWarn("!!#### pos : %d", _pos);
    }
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
    QByteArray mbulk;
    mbulk += "*";
    mbulk += QByteArray::number(data.count());
    mbulk += CRLF;
    for (auto &d : data) {
        mbulk += toBulk(d);
    }
    return mbulk;
}


bool TRedisDriver::waitForState(int state, int msecs)
{
    QEventLoop eventLoop;
    QElapsedTimer timer;
    timer.start();

    while (_client->state() != state) {
        if (timer.elapsed() >= msecs) {
            tSystemWarn("waitForState timeout.  current state:%d  timeout:%d", _client->state(), msecs);
            return false;
        }

        if (_client->error() >= 0) {
            tSystemWarn("waitForState : Error detected.  current state:%d  error:%d", _client->state(), _client->error());
            return false;
        }

        std::this_thread::yield();  // context switch
        while (eventLoop.processEvents()) {}
    }
    return true;
}


void TRedisDriver::moveToThread(QThread *thread)
{
    if (_client->thread() == thread) {
        return;
    }
//return;  // ###################TODO

    // if (_client->isOpen()) {
    //     int socket = TApplicationServerBase::duplicateSocket(_client->socketDescriptor());
    //     delete _client;
    //     _client = new QTcpSocket();
    //     _client->setSocketDescriptor(socket);
    // }

    TKvsDriver::moveToThread(thread);
    _client->moveToThread(thread);
}
