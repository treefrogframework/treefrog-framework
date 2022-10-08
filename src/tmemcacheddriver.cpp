/* Copyright (c) 2022, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tmemcacheddriver.h"
#include "tsystemglobal.h"
using namespace Tf;


bool TMemcachedDriver::command(const QByteArray &cmd)
{
    QByteArray response;
    return request(cmd, response);
}


bool TMemcachedDriver::request(const QByteArray &command, QByteArray &response)
{
    if (Q_UNLIKELY(!isOpen())) {
        tSystemError("Not open memcached session  [%s:%d]", __FILE__, __LINE__);
        return false;
    }

    bool ret = true;
    bool ok = false;
    tSystemDebug("memcached command: %s", command.data());

    if (!writeCommand(command)) {
        tSystemError("memcached write error  [%s:%d]", __FILE__, __LINE__);
        close();
        return false;
    }

    clearBuffer();
    if (readReply()) {

    } else {
        tSystemError("memcached read error  [%s:%d]", __FILE__, __LINE__);
        close();
        return false;
    }

    return ret;
}


QByteArray TMemcachedDriver::getLine(bool *ok)
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


// QByteArray TMemcachedDriver::parseBulkString(bool *ok)
// {
//     QByteArray str;

//     return str;
// }


// QVariantList TMemcachedDriver::parseArray(bool *ok)
// {
//     QVariantList lst;
//     int startpos = _pos;
//     *ok = false;

//     return lst;
// }


// int TMemcachedDriver::getNumber(bool *ok)
// {
//     int idx = _buffer.indexOf(CRLF, _pos);
//     if (idx < 0) {
//         *ok = false;
//         return 0;
//     }

//     int num = _buffer.mid(_pos, idx - _pos).toInt();
//     _pos = idx + 2;
//     *ok = true;
//     tSystemDebug("getNumber: %d", num);
//     return num;
// }


void TMemcachedDriver::clearBuffer()
{
    _buffer.resize(0);
    _pos = 0;
}
