#pragma once
#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QPair>
#include <TGlobal>


class T_CORE_EXPORT TInternetMessageHeader {
public:
    TInternetMessageHeader() = default;
    explicit TInternetMessageHeader(const QByteArray &str);
    TInternetMessageHeader(const TInternetMessageHeader &other);
    TInternetMessageHeader &operator=(const TInternetMessageHeader &other);
    TInternetMessageHeader(TInternetMessageHeader &&) = default;
    TInternetMessageHeader &operator=(TInternetMessageHeader &&) = default;
    virtual ~TInternetMessageHeader() = default;

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
    int64_t contentLength() const;
    void setContentLength(int64_t len);
    QByteArray date() const;
    void setDate(const QByteArray &date);
    void setDate(const QDateTime &dateTime);
    void setCurrentDate();
    virtual QByteArray toByteArray() const;

protected:
    void parse(const QByteArray &header);

    using RawHeaderPair = QPair<QByteArray, QByteArray>;
    using RawHeaderPairList = QList<RawHeaderPair>;
    RawHeaderPairList _headerPairList;
    mutable int64_t _contentLength {-1};
};
