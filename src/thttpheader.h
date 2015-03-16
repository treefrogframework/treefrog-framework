#ifndef THTTPHEADER_H
#define THTTPHEADER_H

#include <TInternetMessageHeader>
#include <TCookie>


class T_CORE_EXPORT THttpHeader : public TInternetMessageHeader
{
public:
    THttpHeader();
    THttpHeader(const THttpHeader &other);
    THttpHeader(const QByteArray &str);
    virtual ~THttpHeader() { }

    THttpHeader &operator=(const THttpHeader &other);
    virtual QByteArray toByteArray() const;
    virtual int majorVersion() const { return majVersion; }
    virtual int minorVersion() const { return minVersion; }

protected:
    int majVersion;
    int minVersion;
};


class T_CORE_EXPORT THttpRequestHeader : public THttpHeader
{
public:
    THttpRequestHeader();
    THttpRequestHeader(const THttpRequestHeader &other);
    THttpRequestHeader(const QByteArray &str);
    THttpRequestHeader &operator=(const THttpRequestHeader &other);

    const QByteArray &method() const { return reqMethod; }
    const QByteArray &path() const { return reqUri; }
    void setRequest(const QByteArray &method, const QByteArray &path, int majorVer = 1, int minorVer = 1);
    QByteArray cookie(const QString &name) const;
    QList<TCookie> cookies() const;
    virtual QByteArray toByteArray() const;

private:
    QByteArray reqMethod;
    QByteArray reqUri;
};


class T_CORE_EXPORT THttpResponseHeader : public THttpHeader
{
public:
    THttpResponseHeader();
    THttpResponseHeader(const THttpResponseHeader &other);
    THttpResponseHeader(const QByteArray &str);

    int statusCode() const { return statCode; }
    void setStatusLine(int code, const QByteArray &text = QByteArray(), int majorVer = 1, int minorVer = 1);
    virtual QByteArray toByteArray() const;
    THttpResponseHeader &operator=(const THttpResponseHeader &other);

private:
    int statCode;
    QByteArray reasonPhr;
};

#endif // THTTPHEADER_H
