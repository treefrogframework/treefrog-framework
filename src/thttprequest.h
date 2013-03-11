#ifndef THTTPREQUEST_H
#define THTTPREQUEST_H

#include <QByteArray>
#include <QVariant>
#include <QList>
#include <TGlobal>
#include <TMultipartFormData>
#include <TCookieJar>
#include <THttpRequestHeader>


class T_CORE_EXPORT THttpRequest
{
public:
    THttpRequest() { }
    THttpRequest(const THttpRequest &other);
    THttpRequest(const THttpRequestHeader &header, const QByteArray &body);
    THttpRequest(const QByteArray &header, const QByteArray &body);
    THttpRequest(const QByteArray &header, const QString &filePath);
    virtual ~THttpRequest();

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

protected:
    void setRequest(const THttpRequestHeader &header, const QByteArray &body);
    void setRequest(const QByteArray &header, const QByteArray &body);
    void setRequest(const QByteArray &header, const QString &filePath);
    QByteArray boundary() const;

private:
    void parseBody(const QByteArray &body);

    THttpRequestHeader reqHeader;
    QVariantMap queryParams;
    QVariantMap formParams;
    TMultipartFormData multiFormData;

    friend class THttpSocket;
};

Q_DECLARE_METATYPE(THttpRequest)

#endif // THTTPREQUEST_H
