#ifndef THTTPHEADER_H
#define THTTPHEADER_H

#include <TInternetMessageHeader>


class T_CORE_EXPORT THttpHeader : public TInternetMessageHeader
{
public:
    THttpHeader() { }
    THttpHeader(const QByteArray &str);
    virtual ~THttpHeader() { }
    virtual QByteArray toByteArray() const;
    virtual int majorVersion() const = 0;
    virtual int minorVersion() const = 0;
};


class T_CORE_EXPORT THttpRequestHeader : public THttpHeader
{
public:
    THttpRequestHeader();
    THttpRequestHeader(const QByteArray &str);

    const QByteArray &method() const { return reqMethod; }
    const QByteArray &path() const { return reqUri; }
    int majorVersion() const { return majVer; }
    int minorVersion() const { return minVer; }
    void setRequest(const QByteArray &method, const QByteArray &path, int majorVer = 1, int minorVer = 1);
    virtual QByteArray toByteArray() const;

private:
    QByteArray reqMethod;
    QByteArray reqUri;
    int majVer;
    int minVer;
};


class T_CORE_EXPORT THttpResponseHeader : public THttpHeader
{
public:
    THttpResponseHeader();
    THttpResponseHeader(const QByteArray &str);

    int statusCode() const { return statCode; }
    void setStatusLine(int code, const QByteArray &text = QByteArray(), int majorVer = 1, int minorVer = 1);
    int majorVersion() const { return majVer; }
    int minorVersion() const { return minVer; }
    virtual QByteArray toByteArray() const;

private:
    int statCode;
    QByteArray reasonPhr;
    int majVer;
    int minVer;
};

#endif // THTTPHEADER_H
