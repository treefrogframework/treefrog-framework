#pragma once
#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QPair>
#include <TGlobal>


class T_CORE_EXPORT TInternetMessageHeader {
public:
    TInternetMessageHeader() { }
    TInternetMessageHeader(const TInternetMessageHeader &other);
    TInternetMessageHeader(const QByteArray &str);
    virtual ~TInternetMessageHeader() { }

    bool hasRawHeader(const QByteArray &key) const;
    QByteArray rawHeader(const QByteArray &key) const;
    QByteArrayList rawHeaderList() const;
    void setRawHeader(const QByteArray &key, const QByteArray &value);
    void addRawHeader(const QByteArray &key, const QByteArray &value);
    void removeAllRawHeaders(const QByteArray &key);
    void removeRawHeader(const QByteArray &key);
    bool isEmpty() const;
    void clear();

    QByteArray contentType() const;
    void setContentType(const QByteArray &type);
    qint64 contentLength() const;
    void setContentLength(qint64 len);
    QByteArray date() const;
    void setDate(const QByteArray &date);
    void setDate(const QDateTime &dateTime);
    void setCurrentDate();
    virtual QByteArray toByteArray() const;
    TInternetMessageHeader &operator=(const TInternetMessageHeader &other);

protected:
    void parse(const QByteArray &header);

    using RawHeaderPair = QPair<QByteArray, QByteArray>;
    using RawHeaderPairList = QList<RawHeaderPair>;
    RawHeaderPairList _headerPairList;
    mutable qint64 _contentLength {-1};
};

