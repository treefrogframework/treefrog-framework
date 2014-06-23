#ifndef THTTPREQUEST_H
#define THTTPREQUEST_H

#include <QByteArray>
#include <QVariant>
#include <QList>
#include <QHostAddress>
#include <QSharedData>
#include <TGlobal>
#include <TMultipartFormData>
#include <TCookieJar>
#include <THttpRequestHeader>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#endif


class T_CORE_EXPORT THttpRequestData : public QSharedData
{
public:
    THttpRequestData() { }
    THttpRequestData(const THttpRequestData &other);
    ~THttpRequestData() { }

    THttpRequestHeader header;
    QVariantMap queryItems;
    QVariantMap formItems;
    TMultipartFormData multipartFormData;
#if QT_VERSION >= 0x050000
    QJsonDocument jsonData;
#endif
    QHostAddress clientAddress;
};


class T_CORE_EXPORT THttpRequest
{
public:
    THttpRequest();
    THttpRequest(const THttpRequest &other);
    THttpRequest(const THttpRequestHeader &header, const QByteArray &body, const QHostAddress &clientAddress);
    THttpRequest(const QByteArray &header, const QString &filePath, const QHostAddress &clientAddress);
    virtual ~THttpRequest();
    THttpRequest &operator=(const THttpRequest &other);

    const THttpRequestHeader &header() const { return d->header; }
    Tf::HttpMethod method() const;
    Tf::HttpMethod realMethod() const;
    Tf::HttpMethod getHttpMethodOverride() const;
    Tf::HttpMethod queryItemMethod() const;
    QString parameter(const QString &name) const;
    QVariantMap allParameters() const;

    bool hasQuery() const { return !d->queryItems.isEmpty(); }
    bool hasQueryItem(const QString &name) const;
    QString queryItemValue(const QString &name) const;
    QString queryItemValue(const QString &name, const QString &defaultValue) const;
    QStringList allQueryItemValues(const QString &name) const;
    const QVariantMap &queryItems() const { return d->queryItems; }
    bool hasForm() const { return !d->formItems.isEmpty(); }
    bool hasFormItem(const QString &name) const;
    QString formItemValue(const QString &name) const;
    QString formItemValue(const QString &name, const QString &defaultValue) const;
    QStringList allFormItemValues(const QString &name) const;
    QStringList formItemList(const QString &key) const;
    QVariantMap formItems(const QString &key) const;
    const QVariantMap &formItems() const { return d->formItems; }
    TMultipartFormData &multipartFormData() { return d->multipartFormData; }
    QByteArray cookie(const QString &name) const;
    QList<TCookie> cookies() const;
    QHostAddress clientAddress() const { return d->clientAddress; }

#if QT_VERSION >= 0x050000
    bool hasJson() const { return !d->jsonData.isNull(); }
    const QJsonDocument &jsonData() const { return d->jsonData; }
#endif

    static QList<THttpRequest> generate(const QByteArray &byteArray, const QHostAddress &address);

protected:
    QByteArray boundary() const;

private:
    void parseBody(const QByteArray &body, const THttpRequestHeader &header);

    QSharedDataPointer<THttpRequestData> d;
};

Q_DECLARE_METATYPE(THttpRequest)

#endif // THTTPREQUEST_H
