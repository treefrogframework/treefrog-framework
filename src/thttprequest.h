#ifndef THTTPREQUEST_H
#define THTTPREQUEST_H

#include <QByteArray>
#include <QVariant>
#include <QList>
#include <QPair>
#include <QHostAddress>
#include <QSharedData>
#include <TGlobal>
#include <TMultipartFormData>
#include <TCookieJar>
#include <THttpRequestHeader>
#include <QJsonDocument>

class QIODevice;


class T_CORE_EXPORT THttpRequestData : public QSharedData
{
public:
    THttpRequestData() { }
    THttpRequestData(const THttpRequestData &other);
    ~THttpRequestData() { }

    THttpRequestHeader header;
    QByteArray bodyArray;
    QList<QPair<QString, QString>> queryItems;
    QList<QPair<QString, QString>> formItems;
    TMultipartFormData multipartFormData;
    QJsonDocument jsonData;
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
    QStringList queryItemList(const QString &key) const;
    QVariantList queryItemVariantList(const QString &key) const;
    QVariantMap queryItems(const QString &key) const;
    QVariantMap queryItems() const;
    bool hasForm() const { return !d->formItems.isEmpty(); }
    bool hasFormItem(const QString &name) const;
    QString formItemValue(const QString &name) const;
    QString formItemValue(const QString &name, const QString &defaultValue) const;
    QStringList allFormItemValues(const QString &name) const;
    QStringList formItemList(const QString &key) const;
    QVariantList formItemVariantList(const QString &key) const;
    QVariantMap formItems(const QString &key) const;
    QVariantMap formItems() const;
    TMultipartFormData &multipartFormData() { return d->multipartFormData; }
    QByteArray cookie(const QString &name) const;
    QList<TCookie> cookies() const;
    QHostAddress clientAddress() const { return d->clientAddress; }
    QIODevice *rawBody();
    bool hasJson() const { return !d->jsonData.isNull(); }
    const QJsonDocument &jsonData() const { return d->jsonData; }

    static QList<THttpRequest> generate(const QByteArray &byteArray, const QHostAddress &address);

protected:
    QByteArray boundary() const;

    static bool hasItem(const QString &name, const QList<QPair<QString, QString>> &items);
    static QString itemValue(const QString &name, const QString &defaultValue, const QList<QPair<QString, QString>> &items);
    static QStringList allItemValues(const QString &name, const QList<QPair<QString, QString>> &items);
    static QVariantList itemVariantList(const QString &key, const QList<QPair<QString, QString>> &items);
    static QVariantMap itemMap(const QList<QPair<QString, QString>> &items);
    static QVariantMap itemMap(const QString &key, const QList<QPair<QString, QString>> &items);

private:
    void parseBody(const QByteArray &body, const THttpRequestHeader &header);

    QSharedDataPointer<THttpRequestData> d;
    QIODevice *bodyDevide {nullptr};
    friend class TMultipartFormData;
};

Q_DECLARE_METATYPE(THttpRequest)

#endif // THTTPREQUEST_H
