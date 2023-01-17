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
    TAccessLogger &accessLogger() { return _accesslogger; }
    const TAccessLogger &accessLogger() const { return _accesslogger; }
    void release();

private:
    QByteArray _arrayBuffer;
    QFile *_bodyFile {nullptr};
    bool _fileRemove {false};
    TAccessLogger _accesslogger;
    int _startPos {0};

    TSendBuffer(const QByteArray &header, const QFileInfo &file, bool autoRemove, TAccessLogger &&logger);
    TSendBuffer(const QByteArray &header);
    TSendBuffer(int statusCode, const QHostAddress &address, const QByteArray &method);
    TSendBuffer();

    friend class TEpollSocket;
    T_DISABLE_COPY(TSendBuffer)
    T_DISABLE_MOVE(TSendBuffer)
};
