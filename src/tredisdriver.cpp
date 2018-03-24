/* Copyright (c) 2015-2017, AOYAMA Kazuharu
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
using namespace Tf;

const int DEFAULT_PORT = 6379;


TRedisDriver::TRedisDriver()
    : TKvsDriver(), client(nullptr), buffer(), pos(0)
{
    buffer.reserve(1023);
}


TRedisDriver::~TRedisDriver()
{
    close();
    delete client;
}


bool TRedisDriver::isOpen() const
{
    return (client) ? (client->state() == QAbstractSocket::ConnectedState) : false;
}


bool TRedisDriver::open(const QString &, const QString &, const QString &, const QString &host, quint16 port, const QString &)
{
    if (!client) {
        client = new QTcpSocket();
    }

    if (isOpen()) {
        return true;
    }

    if (client->state() != QAbstractSocket::UnconnectedState) {
        return false;
    }

    QString hst = (host.isEmpty()) ? "localhost" : host;

    if (port <= 0) {
        port = DEFAULT_PORT;
    }

    tSystemDebug("Redis open host:%s  port:%d", qPrintable(hst), port);
    client->connectToHost(hst, port);

    bool ret = waitForState(QAbstractSocket::ConnectedState, 5000);
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
    if (client) {
        client->close();
        delete client;
        client = nullptr;
    }
}


bool TRedisDriver::readReply()
{
    if (Q_UNLIKELY(!isOpen())) {
        tSystemError("Not open Redis session  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    if (Q_UNLIKELY(pos > 0)) {
        buffer.remove(0, pos);
        pos = 0;
    }

    QEventLoop eventLoop;
    QElapsedTimer timer;
    timer.start();

    int len = buffer.length();
    for (;;) {
        buffer += client->readAll();
        if (buffer.length() != len) {
            break;
        }

        if (timer.elapsed() >= 2000) {
            tSystemWarn("Read timeout");
            break;
        }

        Tf::msleep(0);  // context switch
        while (eventLoop.processEvents()) {}
    }

    //tSystemDebug("Redis response: %s", buffer.data());
    return (buffer.length() > len);
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
    int startpos = pos;

    QByteArray cmd = toMultiBulk(command);
    //tSystemDebug("Redis command: %s", cmd.data());
    client->write(cmd);
    client->flush();
    clearBuffer();

    for (;;) {
        if (!readReply()) {
            clearBuffer();
            break;
        }

        switch (buffer.at(pos)) {
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
            pos++;
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
            tSystemError("Invalid protocol: %c  [%s:%d]", buffer.at(pos), __FILE__, __LINE__);
            ret = false;
            clearBuffer();
            goto parse_done;
            break;
        }

        if (ok) {
            break;
        } else {
            pos = startpos;
        }
    }

parse_done:
    return ret;
}


QByteArray TRedisDriver::getLine(bool *ok)
{
    int idx = buffer.indexOf(CRLF, pos);
    if (idx < 0) {
        *ok = false;
        return QByteArray();
    }

    QByteArray ret = buffer.mid(pos, idx);
    pos = idx + 2;
    *ok = true;
    return ret;
}


QByteArray TRedisDriver::parseBulkString(bool *ok)
{
    QByteArray str;
    int startpos = pos;

    Q_ASSERT((int)buffer[pos] == BulkString);
    pos++;

    int len = getNumber(ok);
    if (*ok) {
        if (len < -1) {
            tSystemError("Invalid length: %d  [%s:%d]", len, __FILE__, __LINE__);
            *ok = false;
        } else if (len == -1) {
            // null string
            tSystemDebug("Null string parsed");
        } else {
            if (pos + 2 <= buffer.length()) {
                str = (len > 0) ? buffer.mid(pos, len) : QByteArray("");
                pos += len + 2;
            } else {
                *ok = false;
            }
        }
    }

    if (! *ok) {
        pos = startpos;
    }
    return str;
}


QVariantList TRedisDriver::parseArray(bool *ok)
{
    QVariantList lst;
    int startpos = pos;
    *ok = false;

    Q_ASSERT((int)buffer[pos] == Array);
    pos++;

    int count = getNumber(ok);
    while (*ok) {
        switch (buffer[pos]) {
        case BulkString: {
            auto str = parseBulkString(ok);
            if (*ok) {
                lst << str;
            }
            break; }

        case Integer: {
            pos++;
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
        pos = startpos;
    }
    return lst;
}


int TRedisDriver::getNumber(bool *ok)
{
    int num = 0;

    int idx = buffer.indexOf(CRLF, pos);
    if (idx < 0) {
        *ok = false;
        return num;
    }

    int c = 1;
    char d = buffer[pos++];

    if (d == '-') {
        c = -1;
        d = buffer[pos++];
    }

    while (d >= '0' && d <= '9') {
        num *= 10;
        num += d - '0';
        d = buffer[pos++];
    }

    pos = idx + 2;
    *ok = true;
    return num * c;
}


void TRedisDriver::clearBuffer()
{
    buffer.resize(0);
    pos = 0;
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

    while (client->state() != state) {
        if (timer.elapsed() >= msecs) {
            tSystemWarn("waitForState timeout.  current state:%d  timeout:%d", client->state(), msecs);
            return false;
        }

        if (client->error() >= 0) {
            tSystemWarn("waitForState : Error detected.  current state:%d  error:%d", client->state(), client->error());
            return false;
        }

        Tf::msleep(0); // context switch
        while (eventLoop.processEvents()) {}
    }
    return true;
}


void TRedisDriver::moveToThread(QThread *thread)
{
    if (client && client->thread() == thread) {
        return;
    }

    int socket = 0;
    if (isOpen()) {
        socket = TApplicationServerBase::duplicateSocket(client->socketDescriptor());
        delete client;
    }

    client = new QTcpSocket();
    client->moveToThread(thread);

    if (socket > 0) {
        client->setSocketDescriptor(socket);
    }
}
