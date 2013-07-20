#ifndef THTTPSENDBUFFER_H
#define THTTPSENDBUFFER_H

#include <QByteArray>
#include <TGlobal>
#include <TAccessLog>

class QFile;
class QFileInfo;
class THttpHeader;


class T_CORE_EXPORT THttpSendBuffer
{
public:
    THttpSendBuffer(const QByteArray &header, const QFileInfo &file, bool autoRemove, const TAccessLog &log);
    ~THttpSendBuffer();

    bool atEnd() const;
    int read(char *data, int maxSize);
    TAccessLog &accessLog() { return accesslog; }
    const TAccessLog &accessLog() const { return accesslog; }
    void release();

private:
    QByteArray arrayBuffer;
    QFile* bodyFile;
    bool fileRemove;
    TAccessLog accesslog;
    int arraySentSize;

    Q_DISABLE_COPY(THttpSendBuffer)
};

#endif // THTTPSENDBUFFER_H
