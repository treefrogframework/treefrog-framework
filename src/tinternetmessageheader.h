#ifndef TINTERNETMESSAGEHEADER_H
#define TINTERNETMESSAGEHEADER_H

#include <QList>
#include <QPair>
#include <QByteArray>
#include <QDateTime>
#include <TGlobal>


class T_CORE_EXPORT TInternetMessageHeader
{
public:
    TInternetMessageHeader() { }
    TInternetMessageHeader(const TInternetMessageHeader &other);
    TInternetMessageHeader(const QByteArray &str);
    virtual ~TInternetMessageHeader() { }

    bool hasRawHeader(const QByteArray &key) const;
    QByteArray rawHeader(const QByteArray &key) const;
    QList<QByteArray> rawHeaderList() const;
    void setRawHeader(const QByteArray &key, const QByteArray &value);
    void addRawHeader(const QByteArray &key, const QByteArray &value);
    void removeAllRawHeaders(const QByteArray &key);
    void removeRawHeader(const QByteArray &key);
    bool isEmpty() const;
    void clear();

    QByteArray contentType() const;
    void setContentType(const QByteArray &type);
    uint contentLength() const;
    void setContentLength(int len);
    QByteArray date() const;
    void setDate(const QByteArray &date);
    void setDate(const QDateTime &dateTime);
    //void setDateUTC(const QDateTime &utc);
    void setCurrentDate();
    virtual QByteArray toByteArray() const;
    TInternetMessageHeader &operator=(const TInternetMessageHeader &other);

protected:
    void parse(const QByteArray &header);

    typedef QPair<QByteArray, QByteArray> RawHeaderPair;
    typedef QList<RawHeaderPair> RawHeaderPairList;
    RawHeaderPairList headerPairList;
};

#endif // TINTERNETMESSAGEHEADER_H
