#ifndef THTTPREQUEST_H
#define THTTPREQUEST_H

#include <QByteArray>
#include <QVariant>
#include <QList>
#include <QHostAddress>
#include <TGlobal>
#include <TMultipartFormData>
#include <TCookieJar>
#include <THttpRequestHeader>

#if QT_VERSION >= 0x050000
#include <QJsonDocument>
#endif


class T_CORE_EXPORT THttpRequest
{
public:
    THttpRequest() { }
    THttpRequest(const THttpRequest &other);
    THttpRequest(const THttpRequestHeader &header, const QByteArray &body);
    THttpRequest(const QByteArray &header, const QByteArray &body);
    THttpRequest(const QByteArray &header, const QString &filePath);
    THttpRequest(const QByteArray &byteArray, const QHostAddress &clientAddress);
    virtual ~THttpRequest();
    THttpRequest &operator=(const THttpRequest &other);

    const THttpRequestHeader &header() const { return reqHeader; }
    Tf::HttpMethod method() const;
    QString parameter(const QString &name) const;
    QVariantMap allParameters() const;

    bool hasQuery() const { return !queryParams.isEmpty(); }
    bool hasQueryItem(const QString &name) const;
    QString queryItemValue(const QString &name) const;
    QString queryItemValue(const QString &name, const QString &defaultValue) const;
    QStringList allQueryItemValues(const QString &name) const;
    const QVariantMap &queryItems() const { return queryParams; }
    bool hasForm() const { return !formParams.isEmpty(); }
    bool hasFormItem(const QString &name) const;
    QString formItemValue(const QString &name) const;
    QString formItemValue(const QString &name, const QString &defaultValue) const;
    QStringList allFormItemValues(const QString &name) const;
    QStringList formItemList(const QString &key) const;
    QVariantMap formItems(const QString &key) const;
    const QVariantMap &formItems() const { return formParams; }
    TMultipartFormData &multipartFormData() { return multiFormData; }
    QByteArray cookie(const QString &name) const;
    QList<TCookie> cookies() const;
    QHostAddress clientAddress() const { return clientAddr; }

#if QT_VERSION >= 0x050000
    bool hasJson() const { return !jsondata.isNull(); }
    const QJsonDocument &jsonData() const { return jsondata; }
#endif

protected:
    void setRequest(const THttpRequestHeader &header, const QByteArray &body);
    void setRequest(const QByteArray &header, const QByteArray &body);
    void setRequest(const QByteArray &header, const QString &filePath);
    QByteArray boundary() const;

private:
    void parseBody(const QByteArray &body, const THttpRequestHeader &header);
    void setClientAddress(const QHostAddress &address) { clientAddr = address; }

    THttpRequestHeader reqHeader;
    QVariantMap queryParams;
    QVariantMap formParams;
    TMultipartFormData multiFormData;
#if QT_VERSION >= 0x050000
    QJsonDocument jsondata;
#endif
    QHostAddress clientAddr;

    friend class THttpSocket;
    friend class TWorkerStarter;
};

Q_DECLARE_METATYPE(THttpRequest)

#endif // THTTPREQUEST_H
