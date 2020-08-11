#pragma once
#include <QByteArray>
#include <QDateTime>
#include <QPair>
#include <QStringList>
#include <QVariant>
#include <TGlobal>

class QTextCodec;


class T_CORE_EXPORT THttpUtility {
public:
    static QString fromUrlEncoding(const QByteArray &enc);
    static QByteArray toUrlEncoding(const QString &input, const QByteArray &exclude = "-._");
    static QList<QPair<QString, QString>> fromFormUrlEncoded(const QByteArray &enc);
    static QString htmlEscape(const QString &input, Tf::EscapeFlag flag = Tf::Quotes);
    static QString htmlEscape(int n);
    static QString htmlEscape(uint n);
    static QString htmlEscape(long n);
    static QString htmlEscape(ulong n);
    static QString htmlEscape(qlonglong n);
    static QString htmlEscape(qulonglong n);
    static QString htmlEscape(double n);
    static QString htmlEscape(const char *input, Tf::EscapeFlag flag = Tf::Quotes);
    static QString htmlEscape(const QByteArray &input, Tf::EscapeFlag flag = Tf::Quotes);
    static QString htmlEscape(const QVariant &input, Tf::EscapeFlag flag = Tf::Quotes);
    static QString jsonEscape(const QString &input);
    static QString jsonEscape(const char *input);
    static QString jsonEscape(const QByteArray &input);
    static QString jsonEscape(const QVariant &input);
    static QByteArray toMimeEncoded(const QString &input, const QByteArray &encoding = "UTF-8");
    static QByteArray toMimeEncoded(const QString &input, QTextCodec *codec);
    static QString fromMimeEncoded(const QByteArray &mime);
    static QByteArray getResponseReasonPhrase(int statusCode);
    static QString trimmedQuotes(const QString &string);
    static QByteArray timeZone();
    static QByteArray toHttpDateTimeString(const QDateTime &dateTime);
    static QDateTime fromHttpDateTimeString(const QByteArray &localTime);
    //static QByteArray toHttpDateTimeUTCString(const QDateTime &utc);
    static QDateTime fromHttpDateTimeUTCString(const QByteArray &utc);
    static QByteArray getUTCTimeString();

private:
    THttpUtility();
    T_DISABLE_COPY(THttpUtility)
    T_DISABLE_MOVE(THttpUtility)
};


/*!
  Internal use.
 */
inline QString THttpUtility::htmlEscape(int n)
{
    return QString::number(n);
}

/*!
  Internal use.
 */
inline QString THttpUtility::htmlEscape(uint n)
{
    return QString::number(n);
}

/*!
  Internal use.
 */
inline QString THttpUtility::htmlEscape(long n)
{
    return QString::number(n);
}

/*!
  Internal use.
 */
inline QString THttpUtility::htmlEscape(ulong n)
{
    return QString::number(n);
}

/*!
  Internal use.
 */
inline QString THttpUtility::htmlEscape(qlonglong n)
{
    return QString::number(n);
}

/*!
  Internal use.
 */
inline QString THttpUtility::htmlEscape(qulonglong n)
{
    return QString::number(n);
}

/*!
  Internal use.
 */
inline QString THttpUtility::htmlEscape(double n)
{
    return QString::number(n);
}

/*!
 * Returns a string that has quotes removed from the start and
 * the end.
 */
inline QString THttpUtility::trimmedQuotes(const QString &string)
{
    QString s = string.trimmed();
    if (s.length() > 1) {
        if ((s.startsWith('"') && s.endsWith('"'))
            || (s.startsWith('\'') && s.endsWith('\''))) {
            return s.mid(1, s.length() - 2);
        }
    }
    return s;
}

