#pragma once
#include <TCookie>
#include <TInternetMessageHeader>


class T_CORE_EXPORT THttpHeader : public TInternetMessageHeader {
public:
    THttpHeader();
    THttpHeader(const THttpHeader &other);
    THttpHeader(const QByteArray &str);
    virtual ~THttpHeader() { }

    THttpHeader &operator=(const THttpHeader &other);
    virtual QByteArray toByteArray() const;
    virtual int majorVersion() const { return _majorVersion; }
    virtual int minorVersion() const { return _minorVersion; }

protected:
    int _majorVersion {1};
    int _minorVersion {1};
};


class T_CORE_EXPORT THttpRequestHeader : public THttpHeader {
public:
    THttpRequestHeader();
    THttpRequestHeader(const THttpRequestHeader &other);
    THttpRequestHeader(const QByteArray &str);
    THttpRequestHeader &operator=(const THttpRequestHeader &other);

    const QByteArray &method() const { return _reqMethod; }
    const QByteArray &path() const { return _reqUri; }
    void setRequest(const QByteArray &method, const QByteArray &path, int majorVer = 1, int minorVer = 1);
    QByteArray cookie(const QString &name) const;
    QList<TCookie> cookies() const;
    virtual QByteArray toByteArray() const;

private:
    QByteArray _reqMethod;
    QByteArray _reqUri;
};


class T_CORE_EXPORT THttpResponseHeader : public THttpHeader {
public:
    THttpResponseHeader();
    THttpResponseHeader(const THttpResponseHeader &other);
    THttpResponseHeader(const QByteArray &str);

    int statusCode() const { return _statusCode; }
    void setStatusLine(int code, const QByteArray &text = QByteArray(), int majorVer = 1, int minorVer = 1);
    virtual QByteArray toByteArray() const;
    THttpResponseHeader &operator=(const THttpResponseHeader &other);

private:
    int _statusCode {0};
    QByteArray _reasonPhrase;
};

