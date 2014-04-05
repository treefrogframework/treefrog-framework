#ifndef THTTPUTILITY_H
#define THTTPUTILITY_H

#include <QByteArray>
#include <QStringList>
#include <QDateTime>
#include <QVariant>
#include <TGlobal>

class QTextCodec;


class T_CORE_EXPORT THttpUtility
{
public:
    static QString fromUrlEncoding(const QByteArray &enc);
    static QByteArray toUrlEncoding(const QString &input, const QByteArray &exclude = "-._");
    static QString htmlEscape(const QString &input, Tf::EscapeFlag flag = Tf::Quotes);
    static QString htmlEscape(int n, Tf::EscapeFlag flag = Tf::Quotes);
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
    Q_DISABLE_COPY(THttpUtility)
};


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

#endif // THTTPUTILITY_H
