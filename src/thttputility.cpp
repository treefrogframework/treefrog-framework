/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QHash>
#include <QTextCodec>
#include <QLocale>
#include "tsystemglobal.h"
#include "thttputility.h"
#if defined(Q_OS_WIN)
#include <qt_windows.h>
#else
#include <time.h>
#endif

#define HTTP_DATE_TIME_FORMAT "ddd, d MMM yyyy hh:mm:ss"

typedef QHash<int, QByteArray> IntHash;

Q_GLOBAL_STATIC_WITH_INITIALIZER(IntHash, reasonPhrase,
{
    // Informational 1xx
    x->insert(Tf::Continue, "Continue");
    x->insert(Tf::SwitchingProtocols, "Switching Protocols");
    // Successful 2xx
    x->insert(Tf::OK, "OK");
    x->insert(Tf::Created, "Created");
    x->insert(Tf::Accepted, "Accepted");
    x->insert(Tf::NonAuthoritativeInformation, "Non-Authoritative Information");
    x->insert(Tf::NoContent, "No Content");
    x->insert(Tf::ResetContent, "Reset Content");
    x->insert(Tf::PartialContent, "Partial Content");
    // Redirection 3xx
    x->insert(Tf::MultipleChoices, "Multiple Choices");
    x->insert(Tf::MovedPermanently, "Moved Permanently");
    x->insert(Tf::Found, "Found");
    x->insert(Tf::SeeOther, "See Other");
    x->insert(Tf::NotModified, "Not Modified");
    x->insert(Tf::UseProxy, "Use Proxy");
    x->insert(Tf::TemporaryRedirect, "Temporary Redirect");
    // Client Error 4xx
    x->insert(Tf::BadRequest, "Bad Request");
    x->insert(Tf::Unauthorized, "Unauthorized");
    x->insert(Tf::PaymentRequired, "Payment Required");
    x->insert(Tf::Forbidden, "Forbidden");
    x->insert(Tf::NotFound, "Not Found");
    x->insert(Tf::MethodNotAllowed, "Method Not Allowed");
    x->insert(Tf::NotAcceptable, "Not Acceptable");
    x->insert(Tf::ProxyAuthenticationRequired, "Proxy Authentication Required");
    x->insert(Tf::RequestTimeout, "Request Timeout");
    x->insert(Tf::Conflict, "Conflict");
    x->insert(Tf::Gone, "Gone");
    x->insert(Tf::LengthRequired, "Length Required");
    x->insert(Tf::PreconditionFailed, "Precondition Failed");
    x->insert(Tf::RequestEntityTooLarge, "Request Entity Too Large");
    x->insert(Tf::RequestURITooLong, "Request-URI Too Long");
    x->insert(Tf::UnsupportedMediaType, "Unsupported Media Type");
    x->insert(Tf::RequestedRangeNotSatisfiable, "Requested Range Not Satisfiable");
    x->insert(Tf::ExpectationFailed, "Expectation Failed");
    // Server Error 5xx
    x->insert(Tf::InternalServerError, "Internal Server Error");
    x->insert(Tf::NotImplemented, "Not Implemented");
    x->insert(Tf::BadGateway, "Bad Gateway");
    x->insert(Tf::ServiceUnavailable, "Service Unavailable");
    x->insert(Tf::GatewayTimeout, "Gateway Timeout");
    x->insert(Tf::HTTPVersionNotSupported, "HTTP Version Not Supported");
});

/*!
  \class THttpUtility
  \brief The THttpUtility class contains utility functions.
*/


QString THttpUtility::fromUrlEncoding(const QByteArray &input)
{
    QByteArray d = input;
    d = QByteArray::fromPercentEncoding(d.replace("+", "%20"));
    return QString::fromUtf8(d.constData(), d.length());
}


QByteArray THttpUtility::toUrlEncoding(const QString &string, const QByteArray &exclude)
{
    return string.toUtf8().toPercentEncoding(exclude, "~").replace("%20", "+");
}


QString THttpUtility::htmlEscape(const QString &str, Tf::EscapeFlag flag)
{
    const QLatin1Char amp('&');
    const QLatin1Char lt('<');
    const QLatin1Char gt('>');
    const QLatin1Char dquot('"');
    const QLatin1Char squot('\'');
    const QLatin1String eamp("&amp;");
    const QLatin1String elt("&lt;");
    const QLatin1String egt("&gt;");
    const QString edquot("&quot;");
    const QString esquot("&#039;");

    QString escaped;
    escaped.reserve(int(str.length() * 1.1));
    for (int i = 0; i < str.length(); ++i) {
        if (str.at(i) == amp) {
            escaped += eamp;
        } else if (str.at(i) == lt) {
            escaped += elt;
        } else if (str.at(i) == gt) {
            escaped += egt;
        } else if (str.at(i) == dquot) {
            escaped += (flag == Tf::Compatible || flag == Tf::Quotes) ? edquot : str.at(i);
        } else if (str.at(i) == squot) {
            escaped += (flag == Tf::Quotes) ? esquot : str.at(i);
        } else {
            escaped += str.at(i);
        }
    }
    return escaped;
}


QString THttpUtility::htmlEscape(int n, Tf::EscapeFlag)
{
    return htmlEscape(QString::number(n));
}


QString THttpUtility::htmlEscape(const char *str, Tf::EscapeFlag flag)
{
    return htmlEscape(QString(str), flag);
}


QString THttpUtility::htmlEscape(const QByteArray &str, Tf::EscapeFlag flag)
{
    return htmlEscape(QString(str), flag);
}


QString THttpUtility::htmlEscape(const QVariant &var, Tf::EscapeFlag flag)
{
    return htmlEscape(var.toString(), flag);
}


QString THttpUtility::jsonEscape(const QString &str)
{
    const QLatin1Char amp('&');
    const QLatin1Char lt('<');
    const QLatin1Char gt('>');

    QString escaped;
    escaped.reserve(int(str.length() * 1.1));
    for (int i = 0; i < str.length(); ++i) {
        if (str.at(i) == amp) {
            escaped += QLatin1String("\\u0026");
        } else if (str.at(i) == lt) {
            escaped += QLatin1String("\\u003C");
        } else if (str.at(i) == gt) {
            escaped += QLatin1String("\\u003E");
        } else {
            escaped += str.at(i);
        }
    }
    return escaped;
}


QString THttpUtility::jsonEscape(const char *str)
{
    return jsonEscape(QString(str));
}


QString THttpUtility::jsonEscape(const QByteArray &str)
{
    return jsonEscape(QString(str));
}


QString THttpUtility::jsonEscape(const QVariant &var)
{
    return jsonEscape(var.toString());
}


QByteArray THttpUtility::toMimeEncoded(const QString &text, const QByteArray &encoding)
{
    QTextCodec *codec = QTextCodec::codecForName(encoding);
    return toMimeEncoded(text, codec);
}


QByteArray THttpUtility::toMimeEncoded(const QString &text, QTextCodec *codec)
{
    QByteArray encoded;
    if (!codec)
        return encoded;

    encoded += "=?";
    encoded += codec->name();
    encoded += "?B?";
    encoded += codec->fromUnicode(text).toBase64();
    encoded += "?=";
    return encoded;
}


QString THttpUtility::fromMimeEncoded(const QByteArray &in)
{
    QString text;
    
    if (!in.startsWith("=?"))
        return text;

    int i = 2;
    int j = in.indexOf('?', i);
    if (j > i) {
        QByteArray encoding = in.mid(i, j - i);
        QTextCodec *codec = QTextCodec::codecForName(encoding);
        if (!codec)
            return text;
        
        i = ++j;
        int j = in.indexOf('?', i);
        if (j > i) {
            QByteArray enc = in.mid(i, j - i);
            i = ++j;
            j = in.indexOf("?=", i);
            if (j > i) {
                if (enc == "B" || enc == "b") {
                    QByteArray base = in.mid(i, i - j);
                    text = codec->toUnicode(QByteArray::fromBase64(base));
                } else if (enc == "Q" || enc == "q") {
                    // no implement..
                } else {
                    // bad parameter
                }
            }
        }
    }
    return text;
}


QByteArray THttpUtility::getResponseReasonPhrase(int statusCode)
{
    return reasonPhrase()->value(statusCode);
}


QByteArray THttpUtility::timeZone()
{
    long offset = 0;  // minutes
    
#if defined(Q_OS_WIN)
    TIME_ZONE_INFORMATION tzi;
    memset(&tzi, 0, sizeof(tzi));
    GetTimeZoneInformation(&tzi);
    offset = -tzi.Bias;

#elif defined(Q_OS_UNIX)
    time_t ltime = 0;
    tm *t = 0;
# if defined(_POSIX_THREAD_SAFE_FUNCTIONS)
    tzset();
    tm res;
    t = localtime_r(&ltime, &res);
# else
    t = localtime(&ltime);
# endif // _POSIX_THREAD_SAFE_FUNCTIONS
    offset = t->tm_gmtoff / 60;
#endif // Q_OS_UNIX

    QByteArray tz;
    tz += (offset > 0) ? '+' : '-';
    offset = qAbs(offset);
    tz += QString("%1%2").arg(offset / 60, 2, 10, QLatin1Char('0')).arg(offset % 60, 2, 10, QLatin1Char('0')).toLatin1();
    tSystemDebug("tz: %s", tz.data());
    return tz;
}


QByteArray THttpUtility::toHttpDateTimeString(const QDateTime &localTime)
{
    QByteArray d = QLocale(QLocale::C).toString(localTime, HTTP_DATE_TIME_FORMAT).toLatin1();
    d += ' ';
    d += timeZone();
    return d;
}


QDateTime THttpUtility::fromHttpDateTimeString(const QByteArray &localTime)
{
    QByteArray tz = localTime.mid(localTime.length() - 5).trimmed();
    if (tz != timeZone()) {
        tWarn("Time zone not match: %s", tz.data());
    }
    return QLocale(QLocale::C).toDateTime(localTime.left(localTime.lastIndexOf(' ')), HTTP_DATE_TIME_FORMAT);
}


QByteArray THttpUtility::toHttpDateTimeUTCString(const QDateTime &utc)
{
    QByteArray d = QLocale(QLocale::C).toString(utc, HTTP_DATE_TIME_FORMAT).toLatin1();
    d += " +0000";
    return d;
}


QDateTime THttpUtility::fromHttpDateTimeUTCString(const QByteArray &utc)
{
    if (!utc.endsWith(" +0000") && !utc.endsWith(" GMT")) {
        tWarn("HTTP Date-Time format error: %s", utc.data());
    }
    return QLocale(QLocale::C).toDateTime(utc.left(utc.lastIndexOf(' ')), HTTP_DATE_TIME_FORMAT);
}
