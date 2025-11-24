#pragma once
#include <TCookie>
#include <TInternetMessageHeader>


class T_CORE_EXPORT THttpHeader : public TInternetMessageHeader {
public:
    THttpHeader();
    explicit THttpHeader(const QByteArray &str);
    THttpHeader(const THttpHeader &other) = default;
    THttpHeader &operator=(const THttpHeader &other) = default;
    THttpHeader(THttpHeader &&) = default;
    THttpHeader &operator=(THttpHeader &&) = default;
    virtual ~THttpHeader() = default;

    virtual QByteArray toByteArray() const;
    virtual int majorVersion() const { return _majorVersion; }
    virtual int minorVersion() const { return _minorVersion; }

protected:
    int _majorVersion {1};
    int _minorVersion {1};
};


class T_CORE_EXPORT THttpRequestHeader : public THttpHeader {
public:
    THttpRequestHeader() = default;
    explicit THttpRequestHeader(const QByteArray &str);
    THttpRequestHeader(const THttpRequestHeader &other) = default;
    THttpRequestHeader &operator=(const THttpRequestHeader &other) = default;
    THttpRequestHeader(THttpRequestHeader &&) = default;
    THttpRequestHeader &operator=(THttpRequestHeader &&) = default;

    const QByteArray &method() const { return _reqMethod; }
    const QByteArray &path() const { return _reqUri; }
    void setRequest(const QByteArray &method, const QByteArray &path, int majorVer = 1, int minorVer = 1);
    QByteArray cookie(const QString &name) const;
    QList<TCookie> cookies() const;
    virtual QByteArray toByteArray() const;
    inline bool isEmpty() const { return THttpHeader::isEmpty() && _reqUri.isEmpty(); }

private:
    QByteArray _reqMethod;
    QByteArray _reqUri;
};


class T_CORE_EXPORT THttpResponseHeader : public THttpHeader {
public:
    THttpResponseHeader() = default;
    explicit THttpResponseHeader(const QByteArray &str);
    THttpResponseHeader(const THttpResponseHeader &) = default;
    THttpResponseHeader &operator=(const THttpResponseHeader &) = default;
    THttpResponseHeader(THttpResponseHeader &&) = default;
    THttpResponseHeader &operator=(THttpResponseHeader &&) = default;

    Tf::StatusCode statusCode() const { return static_cast<Tf::StatusCode>(_statusCode); }
    void setStatusLine(Tf::StatusCode code, const QByteArray &text = QByteArray(), int majorVer = 1, int minorVer = 1);
    virtual QByteArray toByteArray() const;
    void clear();

private:
    int _statusCode {0};
    QByteArray _reasonPhrase;
};

