/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <THttpHeader>
using namespace Tf;

/*!
  \class THttpHeader
  \brief The THttpHeader class is the abstract base class of request or response header information for HTTP.
*/

/*!
  Constructor.
*/
THttpHeader::THttpHeader() :
    TInternetMessageHeader()
{
}

/*!
  Copy constructor.
*/
THttpHeader::THttpHeader(const THttpHeader &other) :
    TInternetMessageHeader(*static_cast<const TInternetMessageHeader *>(&other)),
    _majorVersion(other._majorVersion),
    _minorVersion(other._minorVersion)
{
}

/*!
  Constructs an HTTP header by parsing \a str.
*/
THttpHeader::THttpHeader(const QByteArray &str) :
    TInternetMessageHeader()
{
    parse(str);
}

/*!
  Assigns \a other to this HTTP header and returns a reference
  to this HTTP header.
*/
THttpHeader &THttpHeader::operator=(const THttpHeader &other)
{
    TInternetMessageHeader::operator=(*static_cast<const TInternetMessageHeader *>(&other));
    _majorVersion = other._majorVersion;
    _minorVersion = other._minorVersion;
    return *this;
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
THttpRequestHeader::THttpRequestHeader() :
    THttpHeader()
{
}

/*!
  Copy constructor.
*/
THttpRequestHeader::THttpRequestHeader(const THttpRequestHeader &other) :
    THttpHeader(*static_cast<const THttpHeader *>(&other)),
    _reqMethod(other._reqMethod),
    _reqUri(other._reqUri)
{
}

/*!
  Constructs an HTTP request header by parsing \a str.
*/
THttpRequestHeader::THttpRequestHeader(const QByteArray &str)
{
    int i = str.indexOf('\n');
    if (i > 0) {
        // Parses the string
        parse(str.mid(i + 1));

        QByteArray line = str.left(i).trimmed();
        i = line.indexOf(' ');
        if (i > 0) {
            _reqMethod = line.left(i);
            ++i;
            int j = line.indexOf(' ', i);
            if (j > 0) {
                _reqUri = line.mid(i, j - i);
                i = j;
                j = line.indexOf("HTTP/", i);
                if (j > 0 && j + 7 < line.length()) {
                    THttpHeader::_majorVersion = line.mid(j + 5, 1).toInt();
                    THttpHeader::_minorVersion = line.mid(j + 7, 1).toInt();
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
    _reqMethod = method;
    _reqUri = path;
    THttpHeader::_majorVersion = majorVer;
    THttpHeader::_minorVersion = minorVer;
}

/*!
  Returns the cookie associated with the name.
 */
QByteArray THttpRequestHeader::cookie(const QString &name) const
{
    const QList<TCookie> lst = cookies();
    for (auto &c : lst) {
        if (c.name() == name) {
            return c.value();
        }
    }
    return QByteArray();
}

/*!
  Returns the all cookies.
 */
QList<TCookie> THttpRequestHeader::cookies() const
{
    QList<TCookie> result;
    const QByteArrayList cookieStrings = rawHeader(QByteArrayLiteral("Cookie")).split(';');

    result.reserve(cookieStrings.size());
    for (auto &ck : cookieStrings) {
        QByteArray ba = ck.trimmed();
        if (!ba.isEmpty()) {
            result += TCookie::parseCookies(ba);
        }
    }
    return result;
}

/*!
  Returns a byte array representation of the HTTP request header.
*/
QByteArray THttpRequestHeader::toByteArray() const
{
    QByteArray ba;
    QByteArray hdr = THttpHeader::toByteArray();
    ba.reserve(256 + hdr.size());
    ba += _reqMethod;
    ba += ' ';
    ba += _reqUri;
    ba += QByteArrayLiteral(" HTTP/");
    ba += QByteArray::number(majorVersion());
    ba += '.';
    ba += QByteArray::number(minorVersion());
    ba += CRLF;
    ba += hdr;
    return ba;
}

/*!
  Assigns \a other to this HTTP request header and returns a reference
  to this header.
*/
THttpRequestHeader &THttpRequestHeader::operator=(const THttpRequestHeader &other)
{
    THttpHeader::operator=(*static_cast<const THttpHeader *>(&other));
    _reqMethod = other._reqMethod;
    _reqUri = other._reqUri;
    return *this;
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
THttpResponseHeader::THttpResponseHeader() :
    THttpHeader()
{
}

/*!
  Copy constructor.
*/
THttpResponseHeader::THttpResponseHeader(const THttpResponseHeader &other) :
    THttpHeader(*static_cast<const THttpHeader *>(&other)),
    _statusCode(other._statusCode),
    _reasonPhrase(other._reasonPhrase)
{
}

/*!
  Constructs an HTTP response header by parsing \a str.
*/
THttpResponseHeader::THttpResponseHeader(const QByteArray &str) :
    THttpHeader()
{
    int i = str.indexOf('\n');
    if (i > 0) {
        // Parses the string
        parse(str.mid(i + 1));

        QByteArray line = str.left(i).trimmed();
        i = line.indexOf("HTTP/");
        if (i == 0 && line.length() >= 12) {
            THttpHeader::_majorVersion = line.mid(5, 1).toInt();
            THttpHeader::_minorVersion = line.mid(7, 1).toInt();
            if (line[8] == ' ' || line[8] == '\t') {
                _statusCode = line.mid(9, 3).toInt();
            }

            if (line.length() > 13 && (line[12] == ' ' || line[12] == '\t')) {
                _reasonPhrase = line.mid(13).trimmed();
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
    _statusCode = code;
    _reasonPhrase = text;
    THttpHeader::_majorVersion = majorVer;
    THttpHeader::_minorVersion = minorVer;
}

/*!
  Returns a byte array representation of the HTTP response header.
*/
QByteArray THttpResponseHeader::toByteArray() const
{
    QByteArray ba;
    QByteArray hdr = THttpHeader::toByteArray();
    ba.reserve(256 + hdr.size());
    ba += "HTTP/";
    ba += QByteArray::number(majorVersion());
    ba += '.';
    ba += QByteArray::number(minorVersion());
    ba += ' ';
    ba += QByteArray::number(_statusCode);
    ba += ' ';
    ba += _reasonPhrase;
    ba += CRLF;
    ba += hdr;
    return ba;
}


/*!
  Assigns \a other to this HTTP response header and returns a reference
  to this header.
*/
THttpResponseHeader &THttpResponseHeader::operator=(const THttpResponseHeader &other)
{
    THttpHeader::operator=(*static_cast<const THttpHeader *>(&other));
    _statusCode = other._statusCode;
    _reasonPhrase = other._reasonPhrase;
    return *this;
}

/*!
  \fn int THttpResponseHeader::statusCode() const
  Returns the status code of the HTTP response header.
*/
