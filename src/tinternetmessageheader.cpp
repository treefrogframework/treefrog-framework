/* Copyright (c) 2011-2015, AOYAMA Kazuharu
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

/*!
  \fn TInternetMessageHeader::TInternetMessageHeader()
  Constructs an empty Internet message header.
*/

/*!
  Copy constructor.
*/
TInternetMessageHeader::TInternetMessageHeader(const TInternetMessageHeader &other)
    : headerPairList(other.headerPairList)
{ }

/*!
  Constructs an Internet message header by parsing \a str.
*/
TInternetMessageHeader::TInternetMessageHeader(const QByteArray &str)
{
    parse(str);
}

/*!
  Returns true if the Internet message header has an entry with the given
  \a key; otherwise returns false.
*/
bool TInternetMessageHeader::hasRawHeader(const QByteArray &key) const
{
    return !rawHeader(key).isNull();
}

/*!
  Returns the raw value for the entry with the given \a key. If no entry
  has this key, an empty byte array is returned.
*/
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

/*!
  Returns a list of all raw headers.
*/
QList<QByteArray> TInternetMessageHeader::rawHeaderList() const
{
    QList<QByteArray> list;
    for (QListIterator<RawHeaderPair> i(headerPairList); i.hasNext(); ) {
        list << i.next().first;
    }
    return list;
}

/*!
  Sets the raw header \a key to be of value \a value.
  If \a key was previously set, it is overridden.
*/
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

/*!
  Sets the raw header \a key to be of value \a value.
  If \a key was previously set, it is added multiply.
*/
void TInternetMessageHeader::addRawHeader(const QByteArray &key, const QByteArray &value)
{
    if (key.isEmpty() || value.isNull())
        return;

    headerPairList << RawHeaderPair(key, value);
}

/*!
  Returns the value of the header field content-type.
*/
QByteArray TInternetMessageHeader::contentType() const
{
    return rawHeader("Content-Type");
}

/*!
  Sets the value of the header field content-type to \a type.
*/
void TInternetMessageHeader::setContentType(const QByteArray &type)
{
    setRawHeader("Content-Type", type);
}

/*!
  Returns the value of the header field content-length.
*/
uint TInternetMessageHeader::contentLength() const
{
    return rawHeader("Content-Length").toUInt();
}

/*!
  Sets the value of the header field content-length to \a len.
*/
void TInternetMessageHeader::setContentLength(int len)
{
    setRawHeader("Content-Length", QByteArray::number(len));
}

/*!
  Returns the value of the header field date.
*/
QByteArray TInternetMessageHeader::date() const
{
    return rawHeader("Date");
}

/*!
  Sets the value of the header field date to \a date.
*/
void TInternetMessageHeader::setDate(const QByteArray &date)
{
    setRawHeader("Date", date);
}

/*!
  Sets the value of the header field date to the current date/time.
 */
void TInternetMessageHeader::setCurrentDate()
{
    setDate(THttpUtility::getUTCTimeString());
}

/*!
  Sets the value of the header field date to \a localTime
  as the local time on the computer.
*/
void TInternetMessageHeader::setDate(const QDateTime &dateTime)
{
    setRawHeader("Date", THttpUtility::toHttpDateTimeString(dateTime));
}

/*!
  Sets the value of the header field date to \a utc as Coordinated
  Universal Time.
*/
// void TInternetMessageHeader::setDateUTC(const QDateTime &utc)
// {
//     setRawHeader("Date", THttpUtility::toHttpDateTimeUTCString(utc));
// }

/*!
  Returns a byte array representation of the Internet message header.
*/
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

/*!
  Parses the \a header. This function is for internal use only.
*/
void TInternetMessageHeader::parse(const QByteArray &header)
{
    QByteArray field, value;
    int i = 0;
    int headerlen;

    value.reserve(255);

    headerlen = header.indexOf("\r\n\r\n");
    if (headerlen < 0)
        headerlen = header.length();

    while (i < headerlen) {
        int j = header.indexOf(':', i); // field-name
        if (j < 0)
            break;

        field = header.mid(i, j - i).trimmed();

        // any number of LWS is allowed before and after the value
        ++j;
        value.resize(0);
        do {
            i = header.indexOf('\n', j);
            if (i < 0) {
                i = header.length();
            }

            if (!value.isEmpty())
                value += ' ';

            value += header.mid(j, i - j).trimmed();
            j = ++i;
        } while (i < headerlen && (header.at(i) == ' ' || header.at(i) == '\t'));

        headerPairList << qMakePair(field, value);
    }
}

/*!
  Removes all the entries with the key \a key from the HTTP header.
*/
void TInternetMessageHeader::removeAllRawHeaders(const QByteArray &key)
{
    for (QMutableListIterator<RawHeaderPair> i(headerPairList); i.hasNext(); ) {
        RawHeaderPair &p = i.next();
        if (qstricmp(p.first.constData(), key.constData()) == 0) {
            i.remove();
        }
    }
}

/*!
  Removes the entries with the key \a key from the HTTP header.
*/
void TInternetMessageHeader::removeRawHeader(const QByteArray &key)
{
    for (QMutableListIterator<RawHeaderPair> i(headerPairList); i.hasNext(); ) {
        RawHeaderPair &p = i.next();
        if (qstricmp(p.first.constData(), key.constData()) == 0) {
            i.remove();
            break;
        }
    }
}

/*!
  Returns true if the Internet message header is empty; otherwise
  returns false.
 */
bool TInternetMessageHeader::isEmpty() const
{
    return headerPairList.isEmpty();
}

/*!
  Removes all the entries from the Internet message header.
*/
void TInternetMessageHeader::clear()
{
    headerPairList.clear();
}

/*!
  Assigns \a other to this internet message header and returns a reference
  to this header.
*/
TInternetMessageHeader &TInternetMessageHeader::operator=(const TInternetMessageHeader &other)
{
    headerPairList = other.headerPairList;
    return *this;
}
