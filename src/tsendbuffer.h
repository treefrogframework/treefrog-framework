#pragma once
#include <QByteArray>
#include <TAccessLog>
#include <TGlobal>

class QFile;
class QFileInfo;
class QHostAddress;
class THttpHeader;


class T_CORE_EXPORT TSendBuffer {
public:
    ~TSendBuffer();

    bool atEnd() const;
    void *getData(int &size);
    bool seekData(int pos);
    int prepend(const char *data, int maxSize);
    TAccessLogger &accessLogger() { return accesslogger; }
    const TAccessLogger &accessLogger() const { return accesslogger; }
    void release();

private:
    QByteArray arrayBuffer;
    QFile *bodyFile {nullptr};
    bool fileRemove {false};
    TAccessLogger accesslogger;
    int startPos {0};

    TSendBuffer(const QByteArray &header, const QFileInfo &file, bool autoRemove, const TAccessLogger &logger);
    TSendBuffer(const QByteArray &header);
    TSendBuffer(int statusCode, const QHostAddress &address, const QByteArray &method);
    TSendBuffer();

    friend class TEpollSocket;
    T_DISABLE_COPY(TSendBuffer)
    T_DISABLE_MOVE(TSendBuffer)
};

