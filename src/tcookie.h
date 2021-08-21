#pragma once
#include <QByteArray>
#include <QNetworkCookie>
#include <TGlobal>


class TCookie : public QNetworkCookie {
public:
    TCookie(const QByteArray &name = QByteArray(), const QByteArray &value = QByteArray());
    TCookie(const TCookie &other);
    TCookie(const QNetworkCookie &other);
    ~TCookie() { }

    TCookie &operator=(const TCookie &other);
    qint64 maxAge() const { return _maxAge; }
    void setMaxAge(qint64 maxAge) { _maxAge = maxAge; }
    QByteArray sameSite() const { return _sameSite; }
    bool setSameSite(const QByteArray &sameSite);

    void swap(TCookie &other);
    QByteArray toRawForm(QNetworkCookie::RawForm form = QNetworkCookie::Full) const;
    bool operator!=(const TCookie &other) const;
    bool operator==(const TCookie &other) const;

    static QList<TCookie> parseCookies(const QByteArray &cookieString);

private:
    qint64 _maxAge {INT64_MIN};
    QByteArray _sameSite;
};


/*!
  \class TCookie
  \brief The TCookie class holds one network cookie.
  \sa TCookieJar
*/
