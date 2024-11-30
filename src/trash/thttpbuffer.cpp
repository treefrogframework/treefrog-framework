/* Copyright (c) 2013-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TAppSettings>
#include <THttpRequestHeader>
#include "thttpbuffer.h"
#include "tsystemglobal.h"
#include <algorithm>

static int64_t systemLimitBodyBytes = -1;


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
    int size = std::min(httpBuffer.length(), maxSize);
    QByteArray res(size, 0);
    read(res.data(), size);
    return res;
}


int THttpBuffer::read(char *data, int maxSize)
{
    int size = std::min(httpBuffer.length(), maxSize);
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
        if (systemLimitBodyBytes > 0 && httpBuffer.length() > systemLimitBodyBytes) {
            throw ClientErrorException(Tf::RequestEntityTooLarge);  // Request Entity Too Large
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
    if (Q_UNLIKELY(systemLimitBodyBytes < 0)) {
        systemLimitBodyBytes = Tf::appSettings()->value(Tf::LimitRequestBody, "0").toLongLong() * 2;
    }

    if (Q_LIKELY(lengthToRead < 0)) {
        int idx = httpBuffer.indexOf("\r\n\r\n");
        if (idx > 0) {
            THttpRequestHeader header(httpBuffer);

            if (systemLimitBodyBytes > 0 && header.contentLength() > systemLimitBodyBytes) {
                throw ClientErrorException(Tf::RequestEntityTooLarge);  // Request Entity Too Large
            }

            lengthToRead = qMax(idx + 4 + (int64_t)header.contentLength() - httpBuffer.length(), 0LL);
            tSystemDebug("lengthToRead: {}", (int)lengthToRead);
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
