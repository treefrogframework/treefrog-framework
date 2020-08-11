#pragma once
#include <QByteArray>
#include <QDateTime>
#include <TGlobal>
#include <THttpResponseHeader>

class QIODevice;


class T_CORE_EXPORT THttpResponse {
public:
    THttpResponse() { }
    THttpResponse(const THttpResponseHeader &header, const QByteArray &body);
    ~THttpResponse();

    THttpResponseHeader &header() { return resHeader; }
    const THttpResponseHeader &header() const { return resHeader; }
    bool isBodyNull() const;
    void setBody(const QByteArray &body);
    QByteArray body() const;
    void setBodyFile(const QString &filePath);
    QIODevice *bodyIODevice() { return bodyDevice; }
    qint64 bodyLength() const { return (bodyDevice) ? bodyDevice->size() : 0; }

private:
    THttpResponseHeader resHeader;
    QByteArray tmpByteArray;
    QIODevice *bodyDevice {nullptr};

    T_DISABLE_COPY(THttpResponse)
    T_DISABLE_MOVE(THttpResponse)
};

