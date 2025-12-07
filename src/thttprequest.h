#pragma once
#include <TCookieJar>
#include <TGlobal>
#include <THttpRequestHeader>
#include <TMultipartFormData>
#include <QByteArray>
#include <QHostAddress>
#include <QJsonDocument>
#include <QList>
#include <QPair>
#include <QSharedData>
#include <QVariant>
#include <memory>

class TActionContext;
class QIODevice;


class T_CORE_EXPORT THttpRequestData {
public:
    THttpRequestData() = default;
    ~THttpRequestData() = default;
    THttpRequestData(THttpRequestData &&) = default;
    THttpRequestData &operator=(THttpRequestData &&) = default;

    THttpRequestHeader header;
    std::unique_ptr<QByteArray> bodyArray;
    std::unique_ptr<QList<QPair<QString, QString>>> queryItemList;
    std::unique_ptr<QList<QPair<QString, QString>>> formItemList;
    std::unique_ptr<TMultipartFormData> multipartFormData;
    std::unique_ptr<QJsonDocument> jsonData;
    QHostAddress clientAddress;

    T_DISABLE_COPY(THttpRequestData)
};


class T_CORE_EXPORT THttpRequest {
public:
    THttpRequest();
    THttpRequest(const THttpRequestHeader &header, const QByteArray &body, const QHostAddress &clientAddress, TActionContext *context);
    THttpRequest(const QByteArray &header, const QString &filePath, const QHostAddress &clientAddress, TActionContext *context);
    THttpRequest(const THttpRequest &) = delete;
    THttpRequest &operator=(const THttpRequest &) = delete;
    THttpRequest(THttpRequest &&) = default;
    THttpRequest &operator=(THttpRequest &&) = default;
    virtual ~THttpRequest() = default;

    const THttpRequestHeader &header() const { return d->header; }
    Tf::HttpMethod method() const;
    Tf::HttpMethod realMethod() const;
    Tf::HttpMethod getHttpMethodOverride() const;
    Tf::HttpMethod queryItemMethod() const;
    QString parameter(const QString &name) const;
    QVariantMap allParameters() const;

    bool isEmpty() const { return d->header.isEmpty(); }
    bool hasQuery() const { return d->queryItemList && !d->queryItemList->isEmpty(); }
    bool hasQueryItem(const QString &name) const;
    QString queryItemValue(const QString &name) const;
    QString queryItemValue(const QString &name, const QString &defaultValue) const;
    QStringList allQueryItemValues(const QString &name) const;
    QStringList queryItemList(const QString &key) const;
    QVariantList queryItemVariantList(const QString &key) const;
    QVariantMap queryItems(const QString &key) const;
    QVariantMap queryItems() const;
    bool hasForm() const { return d->formItemList && !d->formItemList->isEmpty(); }
    bool hasFormItem(const QString &name) const;
    QString formItemValue(const QString &name) const;
    QString formItemValue(const QString &name, const QString &defaultValue) const;
    QStringList allFormItemValues(const QString &name) const;
    QStringList formItemList(const QString &key) const;
    QVariantList formItemVariantList(const QString &key) const;
    QVariantMap formItems(const QString &key) const;
    QVariantMap formItems() const;
    TMultipartFormData &multipartFormData();
    QByteArray cookie(const QString &name) const;
    QList<TCookie> cookies() const;
    QHostAddress clientAddress() const { return d->clientAddress; }
    QHostAddress originatingClientAddress() const;
    QIODevice *rawBody();
    bool hasJson() const { return d->jsonData && !d->jsonData->isNull(); }
    const QJsonDocument &jsonData() const;

    static THttpRequest generate(QByteArray &byteArray, const QHostAddress &address, TActionContext *context);
    static QList<QPair<QString, QString>> fromQuery(const QString &query);

protected:
    QByteArray boundary() const;
    const QByteArray &bodyArray() const;
    QByteArray &bodyArray();
    const QList<QPair<QString, QString>> &queryItemList() const;
    QList<QPair<QString, QString>> &queryItemList();
    const QList<QPair<QString, QString>> &formItemList() const;
    QList<QPair<QString, QString>> &formItemList();
    QJsonDocument &jsonData();

    static bool hasItem(const QString &name, const QList<QPair<QString, QString>> &items);
    static QString itemValue(const QString &name, const QString &defaultValue, const QList<QPair<QString, QString>> &items);
    static QStringList allItemValues(const QString &name, const QList<QPair<QString, QString>> &items);
    static QVariantList itemVariantList(const QString &key, const QList<QPair<QString, QString>> &items);
    static QVariantMap itemMap(const QList<QPair<QString, QString>> &items);
    static QVariantMap itemMap(const QString &key, const QList<QPair<QString, QString>> &items);

private:
    void parseBody(const QByteArray &body, const THttpRequestHeader &header, TActionContext *context);

    std::unique_ptr<THttpRequestData> d;
    std::unique_ptr<QIODevice> bodyDevice;

    friend class TMultipartFormData;
};

Q_DECLARE_METATYPE(THttpRequest)
