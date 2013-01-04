/* Copyright (c) 2011-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <THttpHeader>

/*!
  \class THttpHeader
  \brief The THttpHeader class is the abstract base class of request or response header information for HTTP.
*/

/*!
  Constructs an HTTP header by parsing \a str.
*/
THttpHeader::THttpHeader(const QByteArray &str)
{
    parse(str);
}

/*!
  Returns a byte array representation of the HTTP header.
*/
QByteArray THttpHeader::toByteArray() const
{
    return TInternetMessageHeader::toByteArray();
}

/*!
  \fn THttpHeader::THttpHeader()
  Constructor.
*/

/*!
  \fn int THttpHeader::majorVersion() const
  Returns the major protocol-version of the HTTP header.
*/

/*!
  \fn int THttpHeader::minorVersion() const
  Returns the minor protocol-version of the HTTP header.
*/


/*!
  \class THttpRequestHeader
  \brief The THttpRequestHeader class contains request header information
         for HTTP.
*/

/*!
  Constructor.
*/
THttpRequestHeader::THttpRequestHeader()
    : majVer(0), minVer(0)
{ }

/*!
  Constructs an HTTP request header by parsing \a str.
*/
THttpRequestHeader::THttpRequestHeader(const QByteArray &str)
{
    int i = str.indexOf('\n');
    if (i > 0) {
        parse(str.mid(i + 1));
        QByteArray line = str.left(i).trimmed();
        i = line.indexOf(' ');
        if (i > 0) {
            reqMethod = line.left(i);
            ++i;
            int j = line.indexOf(' ', i);
            if (j > 0) {
                reqUri = line.mid(i, j - i);
                i = j;
                j = line.indexOf("HTTP/", i);
                if (j > 0 && j + 7 < line.length()) {
                    majVer = line.mid(j + 5, 1).toInt();
                    minVer = line.mid(j + 7, 1).toInt();
                }
            }
        }
    }
}

/*!
  Sets the request method to \a method, the request-URI to \a path and
  the protocol-version to \a majorVer and \a minorVer.
*/
void THttpRequestHeader::setRequest(const QByteArray &method, const QByteArray &path, int majorVer, int minorVer)
{
    reqMethod = method;
    reqUri = path;
    majVer = majorVer;
    minVer = minorVer;
}

/*!
  Returns a byte array representation of the HTTP request header.
*/
QByteArray THttpRequestHeader::toByteArray() const
{
    QByteArray ba;
    ba += reqMethod + ' ' + reqUri + " HTTP/";
    ba += QByteArray::number(majVer);
    ba += '.';
    ba += QByteArray::number(minVer);
    ba += "\r\n";
    ba += THttpHeader::toByteArray();
    return ba;
}

/*!
  \fn const QByteArray &THttpRequestHeader::method() const
  Returns the method of the HTTP request header.
*/

/*!
  \fn const QByteArray &THttpRequestHeader::path() const
  Returns the request-URI of the HTTP request header.
*/


/*!
  \class THttpResponseHeader
  \brief The THttpResponseHeader class contains response header information
         for HTTP.
*/

/*!
  Constructor.
*/
THttpResponseHeader::THttpResponseHeader()
    : statCode(0), majVer(1), minVer(1)
{ }

/*!
  Constructs an HTTP response header by parsing \a str.
*/
THttpResponseHeader::THttpResponseHeader(const QByteArray &str)
    : statCode(0), majVer(1), minVer(1)
{
    int i = str.indexOf('\n');
    if (i > 0) {
        parse(str.mid(i + 1));
        QByteArray line = str.left(i).trimmed();
        i = line.indexOf("HTTP/");
        if (i == 0 && line.length() >= 12) {
            majVer = line.mid(5, 1).toInt();
            minVer = line.mid(7, 1).toInt();
            if (line[8] == ' ' || line[8] == '\t') {
                statCode = line.mid(9, 3).toInt();
            }

            if (line.length() > 13 &&
                (line[12] == ' ' || line[12] == '\t')) {
                reasonPhr = line.mid(13).trimmed();
            } 
        }
    }
}

/*!
  Sets the status code to \a code, the reason phrase to \a text and
  the protocol-version to \a majorVer and \a minorVer.
*/
void THttpResponseHeader::setStatusLine(int code, const QByteArray &text, int majorVer, int minorVer)
{
    statCode = code;
    reasonPhr = text;
    majVer = majorVer;
    minVer = minorVer;
}

/*!
  Returns a byte array representation of the HTTP response header.
*/
QByteArray THttpResponseHeader::toByteArray() const
{
    QByteArray ba;
    ba += "HTTP/";
    ba += QByteArray::number(majVer);
    ba += '.';
    ba += QByteArray::number(minVer);
    ba += ' ';
    ba += QByteArray::number(statCode);
    ba += ' ';
    ba += reasonPhr;
    ba += "\r\n";
    ba += THttpHeader::toByteArray();
    return ba;
}

/*!
  \fn int THttpResponseHeader::statusCode() const
  Returns the status code of the HTTP response header.
*/
