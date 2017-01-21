/* Copyright (c) 2013-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TAppSettings>
#include <THttpRequestHeader>
#include "thttpbuffer.h"
#include "tsystemglobal.h"

static int limitBodyBytes = -1;


THttpBuffer::THttpBuffer()
    : lengthToRead(-1)
{
    httpBuffer.reserve(1023);
}


THttpBuffer::~THttpBuffer()
{ }


THttpBuffer::THttpBuffer(const THttpBuffer &other)
    : httpBuffer(other.httpBuffer),
      lengthToRead(other.lengthToRead)
{ }


THttpBuffer &THttpBuffer::operator=(const THttpBuffer &other)
{
    httpBuffer = other.httpBuffer;
    lengthToRead = other.lengthToRead;
    return *this;
}


QByteArray THttpBuffer::read(int maxSize)
{
    int size = qMin(httpBuffer.length(), maxSize);
    QByteArray res(size, 0);
    read(res.data(), size);
    return res;
}


int THttpBuffer::read(char *data, int maxSize)
{
    int size = qMin(httpBuffer.length(), maxSize);
    memcpy(data, httpBuffer.data(), size);
    httpBuffer.remove(0, size);
    return size;
}


int THttpBuffer::write(const char *data, int len)
{
    httpBuffer.append(data, len);

    if (lengthToRead < 0) {
        parse();
    } else {
        if (limitBodyBytes > 0 && httpBuffer.length() > limitBodyBytes) {
            throw ClientErrorException(413);  // Request Entity Too Large
        }

        lengthToRead = qMax(lengthToRead - len, 0LL);
    }
    return len;
}


int THttpBuffer::write(const QByteArray &byteArray)
{
    return write(byteArray.data(), byteArray.length());
}


void THttpBuffer::parse()
{
    if (Q_UNLIKELY(limitBodyBytes < 0)) {
        limitBodyBytes = Tf::appSettings()->value(Tf::LimitRequestBody, "0").toInt();
    }

    if (Q_LIKELY(lengthToRead < 0)) {
        int idx = httpBuffer.indexOf("\r\n\r\n");
        if (idx > 0) {
            THttpRequestHeader header(httpBuffer);
            tSystemDebug("content-length: %d", header.contentLength());

            if (limitBodyBytes > 0 && header.contentLength() > (uint)limitBodyBytes) {
                throw ClientErrorException(413);  // Request Entity Too Large
            }

            lengthToRead = qMax(idx + 4 + (qint64)header.contentLength() - httpBuffer.length(), 0LL);
            tSystemDebug("lengthToRead: %d", (int)lengthToRead);
        }
    } else {
        tSystemWarn("Unreachable code in normal communication");
    }
}


bool THttpBuffer::canReadHttpRequest() const
{
    return (lengthToRead == 0);
}


void THttpBuffer::clear()
{
    lengthToRead = -1;
    httpBuffer.truncate(0);
    httpBuffer.reserve(1023);
}
