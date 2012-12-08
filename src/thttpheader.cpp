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

THttpHeader::THttpHeader(const QByteArray &str)
{
    parse(str);
}


QByteArray THttpHeader::toByteArray() const
{
    return TInternetMessageHeader::toByteArray();
}

/*!
  \class THttpRequestHeader
  \brief The THttpRequestHeader class contains request header information
         for HTTP.
*/

THttpRequestHeader::THttpRequestHeader()
    : majVer(0), minVer(0)
{ }


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


void THttpRequestHeader::setRequest(const QByteArray &method, const QByteArray &path, int majorVer, int minorVer)
{
    reqMethod = method;
    reqUri = path;
    majVer = majorVer;
    minVer = minorVer;
}


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
  \class THttpResponseHeader
  \brief The THttpResponseHeader class contains response header information
         for HTTP.
*/

THttpResponseHeader::THttpResponseHeader()
    : statCode(0), majVer(1), minVer(1)
{ }


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


void THttpResponseHeader::setStatusLine(int code, const QByteArray &text, int majorVer, int minorVer)
{
    statCode = code;
    reasonPhr = text;
    majVer = majorVer;
    minVer = minorVer;
}


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
