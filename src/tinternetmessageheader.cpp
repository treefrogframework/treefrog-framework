/* Copyright (c) 2011-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TInternetMessageHeader>
#include "tsystemglobal.h"
#include "thttputility.h"

#ifdef Q_OS_WIN
#define CRLF "\n"
#else
#define CRLF "\r\n"
#endif

/*!
  \class TInternetMessageHeader
  \brief The TInternetMessageHeader class contains internet message headers.
*/

TInternetMessageHeader::TInternetMessageHeader(const QByteArray &str)
{
    parse(str);
}


bool TInternetMessageHeader::hasRawHeader(const QByteArray &key) const
{
    for (QListIterator<RawHeaderPair> i(headerPairList); i.hasNext(); ) {
        const RawHeaderPair &p = i.next();
        if (qstricmp(p.first.constData(), key.constData()) == 0) {
            return true;
        }
    }
    return false;
}


QByteArray TInternetMessageHeader::rawHeader(const QByteArray &key) const
{
    for (QListIterator<RawHeaderPair> i(headerPairList); i.hasNext(); ) {
        const RawHeaderPair &p = i.next();
        if (qstricmp(p.first.constData(), key.constData()) == 0) {
            return p.second;
        }
    }
    return QByteArray();
}


QList<QByteArray> TInternetMessageHeader::rawHeaderList() const
{
    QList<QByteArray> list;
    for (QListIterator<RawHeaderPair> i(headerPairList); i.hasNext(); ) {
        list << i.next().first;
    }
    return list;
}


void TInternetMessageHeader::setRawHeader(const QByteArray &key, const QByteArray &value)
{
    if (!hasRawHeader(key)) {
        headerPairList << RawHeaderPair(key, value);
        return;
    }
    
    QByteArray val = value;
    for (QMutableListIterator<RawHeaderPair> i(headerPairList); i.hasNext(); ) {
        RawHeaderPair &p = i.next();
        if (qstricmp(p.first.constData(), key.constData()) == 0) {
            if (val.isNull()) {
                i.remove();
            } else {
                p.second = val;
                val.clear();
            }
        }
    }
}


void TInternetMessageHeader::addRawHeader(const QByteArray &key, const QByteArray &value)
{
    if (key.isEmpty() || value.isNull())
        return;

    headerPairList << RawHeaderPair(key, value);
}


QByteArray TInternetMessageHeader::contentType() const
{
    return rawHeader("Content-Type");
}


void TInternetMessageHeader::setContentType(const QByteArray &type)
{
    setRawHeader("Content-Type", type);
}


uint TInternetMessageHeader::contentLength() const
{
    return rawHeader("Content-Length").toUInt();
}


void TInternetMessageHeader::setContentLength(int len)
{
    setRawHeader("Content-Length", QByteArray::number(len));
}


QByteArray TInternetMessageHeader::date() const
{
    return rawHeader("Date");
}


void TInternetMessageHeader::setDate(const QByteArray &date)
{
    setRawHeader("Date", date); 
}


void TInternetMessageHeader::setDate(const QDateTime &localTime)
{
    setRawHeader("Date", THttpUtility::toHttpDateTimeString(localTime));
}


void TInternetMessageHeader::setDateUTC(const QDateTime &utc)
{
    setRawHeader("Date", THttpUtility::toHttpDateTimeUTCString(utc));
}


QByteArray TInternetMessageHeader::toByteArray() const
{
    QByteArray res;
    for (QListIterator<RawHeaderPair> i(headerPairList); i.hasNext(); ) {
        const RawHeaderPair &p = i.next();
        res += p.first;
        res += ": ";
        res += p.second;
        res += CRLF;
    }

    res += CRLF;
    return res;
}


void TInternetMessageHeader::parse(const QByteArray &header)
{
    int i = 0;
    while (i < header.count()) {
        int j = header.indexOf(':', i); // field-name
        if (j < 0)
            break;

        const QByteArray field = header.mid(i, j - i).trimmed();

        // any number of LWS is allowed before and after the value
        ++j;
        QByteArray value;
        do {
            i = header.indexOf('\n', j);
            if (i < 0) {
                i = header.length();
            }

            if (!value.isEmpty())
                value += ' ';

            value += header.mid(j, i - j).trimmed();
            j = ++i;
        } while (i < header.count() && (header.at(i) == ' ' || header.at(i) == '\t'));
        
        headerPairList << qMakePair(field, value);
    }
}


void TInternetMessageHeader::removeAllRawHeaders(const QByteArray &key)
{
    for (QMutableListIterator<RawHeaderPair> i(headerPairList); i.hasNext(); ) {
        RawHeaderPair &p = i.next();
        if (qstricmp(p.first.constData(), key.constData()) == 0) {
            i.remove();
        }
    }
}


bool TInternetMessageHeader::isEmpty() const
{
    return headerPairList.isEmpty();
}


void TInternetMessageHeader::clear()
{
    headerPairList.clear();
}
