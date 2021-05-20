/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tredisdriver.h"
#include "tsystemglobal.h"
using namespace Tf;


bool TRedisDriver::command(const QString &cmd)
{
    QByteArrayList reqcmd = cmd.trimmed().toUtf8().split(' ');
    reqcmd.removeAll("");
    QVariantList response;
    return request(reqcmd, response);
}


bool TRedisDriver::request(const QByteArrayList &command, QVariantList &response)
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
    if (!writeCommand(cmd)) {
        tSystemError("Redis write error  [%s:%d]", __FILE__, __LINE__);
        close();
        return false;
    }
    clearBuffer();

    for (;;) {
        if (!readReply()) {
            tSystemError("Redis read error   pos:%d  buflen:%lld", _pos, _buffer.length());
            close();
            break;
        }

        switch (_buffer.at(_pos)) {
        case Error:
            ret = false;
            str = getLine(&ok);
            tSystemError("Redis error response: %s", qUtf8Printable(str));
            break;

        case SimpleString:
            str = getLine(&ok);
            tSystemDebug("Redis response: %s", qUtf8Printable(str));
            break;

        case Integer: {
            _pos++;
            int num = getNumber(&ok);
            if (ok) {
                response << num;
            }
            break;
        }

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
            close();
            goto parse_done;
            break;
        }

        if (ok) {
            if (_pos < _buffer.length()) {
                tSystemError("Invalid format  [%s:%d]", __FILE__, __LINE__);
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

    if (!*ok) {
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
            break;
        }

        case Integer: {
            _pos++;
            int num = getNumber(ok);
            if (*ok) {
                lst << num;
            }
            break;
        }

        case Array: {
            auto var = parseArray(ok);
            if (*ok) {
                lst << QVariant(var);
            }
            break;
        }

        default:
            tSystemError("Bad logic  [%s:%d]", __FILE__, __LINE__);
            *ok = false;
            break;
        }

        if (lst.count() >= count) {
            break;
        }
    }

    if (!*ok) {
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


QByteArray TRedisDriver::toMultiBulk(const QByteArrayList &data)
{
    QByteArray mbulk("*");
    mbulk += QByteArray::number(data.count());
    mbulk += CRLF;
    for (auto &d : data) {
        mbulk += toBulk(d);
    }
    return mbulk;
}
