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
    static QString fromUrlEncoding(const QByteArray &input);
    static QByteArray toUrlEncoding(const QString &string, const QByteArray &exclude = "-._");
    static QString htmlEscape(const QString &str, Tf::EscapeFlag flag = Tf::Quotes);
    static QString htmlEscape(int n, Tf::EscapeFlag flag = Tf::Quotes);
    static QString htmlEscape(const char *str, Tf::EscapeFlag flag = Tf::Quotes);
    static QString htmlEscape(const QByteArray &str, Tf::EscapeFlag flag = Tf::Quotes);
    static QString htmlEscape(const QVariant &var, Tf::EscapeFlag flag = Tf::Quotes);
    static QString jsonEscape(const QString &str);
    static QString jsonEscape(const char *str);
    static QString jsonEscape(const QByteArray &str);
    static QString jsonEscape(const QVariant &var);
    static QByteArray toMimeEncoded(const QString &text, const QByteArray &encoding = "UTF-8");
    static QByteArray toMimeEncoded(const QString &text, QTextCodec *codec);
    static QString fromMimeEncoded(const QByteArray &in);
    static QByteArray getResponseReasonPhrase(int statusCode);
    static QString trimmedQuotes(const QString &string);
    static QByteArray timeZone();
    static QByteArray toHttpDateTimeString(const QDateTime &localTime);
    static QDateTime fromHttpDateTimeString(const QByteArray &localTime);
    static QByteArray toHttpDateTimeUTCString(const QDateTime &utc);
    static QDateTime fromHttpDateTimeUTCString(const QByteArray &utc);

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
