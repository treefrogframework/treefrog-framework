/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "thttputility.h"
#include "tsystemglobal.h"
#include <QLocale>
#include <QMap>
#include <QUrl>
#include <QStringEncoder>

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#else
#include <ctime>
#endif

constexpr auto HTTP_DATE_TIME_FORMAT = "ddd, d MMM yyyy hh:mm:ss";

const QMap<int, QByteArray> reasonPhrase = {
    // Informational 1xx
    {Tf::Continue, "Continue"},
    {Tf::SwitchingProtocols, "Switching Protocols"},
    // Successful 2xx
    {Tf::OK, "OK"},
    {Tf::Created, "Created"},
    {Tf::Accepted, "Accepted"},
    {Tf::NonAuthoritativeInformation, "Non-Authoritative Information"},
    {Tf::NoContent, "No Content"},
    {Tf::ResetContent, "Reset Content"},
    {Tf::PartialContent, "Partial Content"},
    // Redirection 3xx
    {Tf::MultipleChoices, "Multiple Choices"},
    {Tf::MovedPermanently, "Moved Permanently"},
    {Tf::Found, "Found"},
    {Tf::SeeOther, "See Other"},
    {Tf::NotModified, "Not Modified"},
    {Tf::UseProxy, "Use Proxy"},
    {Tf::TemporaryRedirect, "Temporary Redirect"},
    // Client Error 4xx
    {Tf::BadRequest, "Bad Request"},
    {Tf::Unauthorized, "Unauthorized"},
    {Tf::PaymentRequired, "Payment Required"},
    {Tf::Forbidden, "Forbidden"},
    {Tf::NotFound, "Not Found"},
    {Tf::MethodNotAllowed, "Method Not Allowed"},
    {Tf::NotAcceptable, "Not Acceptable"},
    {Tf::ProxyAuthenticationRequired, "Proxy Authentication Required"},
    {Tf::RequestTimeout, "Request Timeout"},
    {Tf::Conflict, "Conflict"},
    {Tf::Gone, "Gone"},
    {Tf::LengthRequired, "Length Required"},
    {Tf::PreconditionFailed, "Precondition Failed"},
    {Tf::RequestEntityTooLarge, "Request Entity Too Large"},
    {Tf::RequestURITooLong, "Request-URI Too Long"},
    {Tf::UnsupportedMediaType, "Unsupported Media Type"},
    {Tf::RequestedRangeNotSatisfiable, "Requested Range Not Satisfiable"},
    {Tf::ExpectationFailed, "Expectation Failed"},
    // Server Error 5xx
    {Tf::InternalServerError, "Internal Server Error"},
    {Tf::NotImplemented, "Not Implemented"},
    {Tf::BadGateway, "Bad Gateway"},
    {Tf::ServiceUnavailable, "Service Unavailable"},
    {Tf::GatewayTimeout, "Gateway Timeout"},
    {Tf::HTTPVersionNotSupported, "HTTP Version Not Supported"},
};


/*!
  \class THttpUtility
  \brief The THttpUtility class contains utility functions.
*/

/*!
  Returns a decoded copy of \a enc. \a enc is first decoded from URL
  encoding, then converted from UTF-8 to unicode.
  @sa toUrlEncoding(const QString &, const QByteArray &)
*/
QString THttpUtility::fromUrlEncoding(const QByteArray &enc)
{
    QByteArray d = enc;
    d = QByteArray::fromPercentEncoding(d.replace("+", "%20"));
    return QString::fromUtf8(d.constData(), d.length());
}

/*!
  Returns an encoded copy of \a input. \a input is first converted to UTF-8,
  and all ASCII-characters that are not in the unreserved group are URL
  encoded.
  @sa fromUrlEncoding(const QByteArray &)
*/
QByteArray THttpUtility::toUrlEncoding(const QString &input, const QByteArray &exclude)
{
    return input.toUtf8().toPercentEncoding(exclude, "~").replace("%20", "+");
}


QList<QPair<QString, QString>> THttpUtility::fromFormUrlEncoded(const QByteArray &enc)
{
    QList<QPair<QString, QString>> items;

    if (!enc.isEmpty()) {
        const QByteArrayList formdata = enc.split('&');
        for (auto &f : formdata) {
            QByteArrayList kv = f.split('=');
            if (!kv.value(0).isEmpty()) {
                QString key = THttpUtility::fromUrlEncoding(kv.value(0));
                QString val = THttpUtility::fromUrlEncoding(kv.value(1));
                items << QPair<QString, QString>(key, val);
            }
        }
    }
    return items;
}

/*!
  Returns a converted copy of \a input. All applicable characters in \a input
  are converted to HTML entities. The conversions performed are:
  - & (ampersand) becomes &amp;amp;.
  - " (double quote) becomes &amp;quot; when Tf::Compatible or Tf::Quotes
    is set.
  - ' (single quote) becomes &amp;#039; only when Tf::Quotes is set.
  - < (less than) becomes &amp;lt;.
  - > (greater than) becomes &amp;gt;.
*/
QString THttpUtility::htmlEscape(const QString &input, Tf::EscapeFlag flag)
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
    escaped.reserve(int(input.length() * 1.1));
    for (int i = 0; i < input.length(); ++i) {
        if (input.at(i) == amp) {
            escaped += eamp;
        } else if (input.at(i) == lt) {
            escaped += elt;
        } else if (input.at(i) == gt) {
            escaped += egt;
        } else if (input.at(i) == dquot) {
            escaped += (flag == Tf::Compatible || flag == Tf::Quotes) ? edquot : input.at(i);
        } else if (input.at(i) == squot) {
            escaped += (flag == Tf::Quotes) ? esquot : input.at(i);
        } else {
            escaped += input.at(i);
        }
    }
    return escaped;
}

/*!
  This function overloads htmlEscape(const QString &, Tf::EscapeFlag).
*/
QString THttpUtility::htmlEscape(const char *input, Tf::EscapeFlag flag)
{
    return htmlEscape(QString(input), flag);
}

/*!
  This function overloads htmlEscape(const QString &, Tf::EscapeFlag).
*/
QString THttpUtility::htmlEscape(const QByteArray &input, Tf::EscapeFlag flag)
{
    return htmlEscape(QString(input), flag);
}

/*!
  This function overloads htmlEscape(const QString &, Tf::EscapeFlag).
*/
QString THttpUtility::htmlEscape(const QVariant &input, Tf::EscapeFlag flag)
{
    if (input.userType() == QMetaType::QUrl) {
        return htmlEscape(input.toUrl().toString(QUrl::FullyEncoded), flag);
    } else {
        return htmlEscape(input.toString(), flag);
    }
}

/*!
  Returns a converted copy of \a input. All applicable characters in \a input
  are converted to JSON representation. The conversions
  performed are:
  - & (ampersand) becomes \\u0026.
  - < (less than) becomes \\u003C.
  - > (greater than) becomes \\u003E.
*/
QString THttpUtility::jsonEscape(const QString &input)
{
    const QLatin1Char amp('&');
    const QLatin1Char lt('<');
    const QLatin1Char gt('>');

    QString escaped;
    escaped.reserve(int(input.length() * 1.1));
    for (int i = 0; i < input.length(); ++i) {
        if (input.at(i) == amp) {
            escaped += QLatin1String("\\u0026");
        } else if (input.at(i) == lt) {
            escaped += QLatin1String("\\u003C");
        } else if (input.at(i) == gt) {
            escaped += QLatin1String("\\u003E");
        } else {
            escaped += input.at(i);
        }
    }
    return escaped;
}

/*!
  This function overloads jsonEscape(const QString &).
*/
QString THttpUtility::jsonEscape(const char *input)
{
    return jsonEscape(QString(input));
}

/*!
  This function overloads jsonEscape(const QString &).
*/
QString THttpUtility::jsonEscape(const QByteArray &input)
{
    return jsonEscape(QString(input));
}

/*!
  This function overloads jsonEscape(const QString &).
*/
QString THttpUtility::jsonEscape(const QVariant &input)
{
    return jsonEscape(input.toString());
}

/*!
  This function overloads toMimeEncoded(const QString &, QTextCodec *).
  @sa fromMimeEncoded(const QByteArray &)
*/
QByteArray THttpUtility::toMimeEncoded(const QString &input, const QByteArray &encoding)
{
    auto e = QStringConverter::encodingForName(encoding.data());
    QStringConverter::Encoding enc = (e) ? e.value() : QStringConverter::Utf8;
    return toMimeEncoded(input, enc);
}

/*!
  Returns a decoded copy of the MIME-Base64 array \a mime.
  @sa toMimeEncoded(const QString &, QTextCodec *)
*/
QByteArray THttpUtility::toMimeEncoded(const QString &input, QStringConverter::Encoding encoding)
{
    QByteArray encoded;
    QStringEncoder encoder(encoding);

    if (!encoder.isValid()) {
        return encoded;
    }

    QByteArray array = encoder.encode(input);
    encoded += "=?";
    encoded += encoder.name();
    encoded += "?B?";
    encoded += array.toBase64();
    encoded += "?=";
    return encoded;
}


/*!
  Returns a byte array copy of \a input, encoded as MIME-Base64.
  @sa fromMimeEncoded(const QByteArray &)
*/
QString THttpUtility::fromMimeEncoded(const QByteArray &mime)
{
    QString text;

    if (!mime.startsWith("=?"))
        return text;

    int i = 2;
    int j = mime.indexOf('?', i);
    if (j > i) {
        QByteArray encoding = mime.mid(i, j - i);
        auto enc = QStringConverter::encodingForName(encoding);
        if (!enc) {
            return text;
        }
        QStringDecoder decoder(enc.value());

        i = ++j;
        int j = mime.indexOf('?', i);
        if (j > i) {
            QByteArray enc = mime.mid(i, j - i);
            i = ++j;
            j = mime.indexOf("?=", i);
            if (j > i) {
                if (enc == "B" || enc == "b") {
                    QByteArray base = mime.mid(i, i - j);
                    text = decoder.decode(QByteArray::fromBase64(base));
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

/*!
  Returns a reason phrase of the HTTP status code \a statusCode.
*/
QByteArray THttpUtility::getResponseReasonPhrase(int statusCode)
{
    return reasonPhrase.value(statusCode);
}

/*!
  Returns the numeric timezone "[+|-]hhmm" of the current computer
  as a bytes array.
*/
QByteArray THttpUtility::timeZone()
{
    long offset = 0;  // minutes

#if defined(Q_OS_WIN)
    TIME_ZONE_INFORMATION tzi;
    std::memset(&tzi, 0, sizeof(tzi));
    GetTimeZoneInformation(&tzi);
    offset = -tzi.Bias;

#elif defined(Q_OS_UNIX)
    time_t ltime = 0;
    tm *t = 0;
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS)
    tzset();
    tm res;
    t = localtime_r(&ltime, &res);
#else
    t = localtime(&ltime);
#endif  // _POSIX_THREAD_SAFE_FUNCTIONS
    offset = t->tm_gmtoff / 60;
#endif  // Q_OS_UNIX

    QByteArray tz;
    tz += (offset > 0) ? '+' : '-';
    offset = qAbs(offset);
    tz += QString("%1%2").arg(offset / 60, 2, 10, QLatin1Char('0')).arg(offset % 60, 2, 10, QLatin1Char('0')).toLatin1();
    tSystemDebug("tz: {}", (const char*)tz.data());
    return tz;
}

/*!
  Returns a byte array for Date field of an HTTP header, containing
  the datetime equivalent of \a localTime.
*/
QByteArray THttpUtility::toHttpDateTimeString(const QDateTime &dateTime)
{
    QByteArray d = QLocale(QLocale::C).toString(dateTime, HTTP_DATE_TIME_FORMAT).toLatin1();
    d += ' ';

    switch (dateTime.timeSpec()) {
    case Qt::LocalTime:
        d += timeZone();
        break;

    case Qt::UTC:
        d += "+0000";
        break;

    default:
        Tf::warn("Invalid time specification");
        break;
    }
    return d;
}

/*!
  Parses the HTTP datetime array given in \a localTime and returns
  the datetime.
*/
QDateTime THttpUtility::fromHttpDateTimeString(const QByteArray &localTime)
{
    QByteArray tz = localTime.mid(localTime.length() - 5).trimmed();
    if (!tz.contains("GMT") && tz != timeZone()) {
        Tf::warn("Time zone not match: {}", (const char *)tz.data());
    }
    return QLocale(QLocale::C).toDateTime(localTime.left(localTime.lastIndexOf(' ')), HTTP_DATE_TIME_FORMAT);
}

/*!
  Returns a byte array for Date field of an HTTP header, containing
  the UTC datetime equivalent of \a utc.
*/
// QByteArray THttpUtility::toHttpDateTimeUTCString(const QDateTime &utc)
// {
//     QByteArray d = QLocale(QLocale::C).toString(utc, HTTP_DATE_TIME_FORMAT).toLatin1();
//     d += " +0000";
//     return d;
// }

/*!
  Parses the UTC datetime array given in \a utc and returns
  the datetime.
*/
QDateTime THttpUtility::fromHttpDateTimeUTCString(const QByteArray &utc)
{
    if (!utc.endsWith(" +0000") && !utc.endsWith(" GMT")) {
        Tf::warn("HTTP Date-Time format error: {}", utc.data());
    }
    return QLocale(QLocale::C).toDateTime(utc.left(utc.lastIndexOf(' ')), HTTP_DATE_TIME_FORMAT);
}


QByteArray THttpUtility::getUTCTimeString()
{
    static const char *DAY[] = {"Sun, ", "Mon, ", "Tue, ", "Wed, ", "Thu, ", "Fri, ", "Sat, "};
    static const char *MONTH[] = {"Jan ", "Feb ", "Mar ", "Apr ", "May ", "Jun ", "Jul ", "Aug ", "Sep ", "Oct ", "Nov ", "Dec "};

    QByteArray utcTime;
    utcTime.reserve(31);

#if defined(Q_OS_WIN)
    SYSTEMTIME st;
    std::memset(&st, 0, sizeof(SYSTEMTIME));
    GetSystemTime(&st);
    utcTime += DAY[st.wDayOfWeek];
    utcTime += QByteArray::number(st.wDay).rightJustified(2, '0');
    utcTime += ' ';
    utcTime += MONTH[st.wMonth - 1];
    utcTime += QByteArray::number(st.wYear);
    utcTime += ' ';
    utcTime += QByteArray::number(st.wHour).rightJustified(2, '0');
    utcTime += ':';
    utcTime += QByteArray::number(st.wMinute).rightJustified(2, '0');
    utcTime += ':';
    utcTime += QByteArray::number(st.wSecond).rightJustified(2, '0');
    utcTime += " GMT";
#elif defined(Q_OS_UNIX)
    time_t gtime = 0;
    tm *t = 0;
    time(&gtime);
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS)
    tzset();
    tm res;
    t = gmtime_r(&gtime, &res);
#else
    t = gmtime(&gtime);
#endif  // _POSIX_THREAD_SAFE_FUNCTIONS
    utcTime += DAY[t->tm_wday];
    utcTime += QByteArray::number(t->tm_mday).rightJustified(2, '0');
    utcTime += ' ';
    utcTime += MONTH[t->tm_mon];
    utcTime += QByteArray::number(t->tm_year + 1900);
    utcTime += ' ';
    utcTime += QByteArray::number(t->tm_hour).rightJustified(2, '0');
    utcTime += ':';
    utcTime += QByteArray::number(t->tm_min).rightJustified(2, '0');
    utcTime += ':';
    utcTime += QByteArray::number(t->tm_sec).rightJustified(2, '0');
    utcTime += " GMT";
#endif  // Q_OS_UNIX

    return utcTime;
}
