#pragma once
#include "tatomic.h"
#include "tdatabasecontext.h"
#include <QMap>
#include <QStringList>
#include <TAccessLog>
#include <TGlobal>

class QIODevice;
class QHostAddress;
class THttpResponseHeader;
class THttpSocket;
class THttpRequest;
class THttpResponse;
class TApplicationServer;
class TTemporaryFile;
class TActionController;


class T_CORE_EXPORT TActionContext : public TDatabaseContext {
public:
    TActionContext();
    virtual ~TActionContext();

    TTemporaryFile &createTemporaryFile();
    void stop() { stopped = true; }
    QHostAddress clientAddress() const;
    QHostAddress originatingClientAddress() const;
    const TActionController *currentController() const { return currController; }
    THttpRequest &httpRequest() { return *httpReq; }
    const THttpRequest &httpRequest() const { return *httpReq; }
    TCache *cache();

protected:
    void execute(THttpRequest &request, int sid);
    void release();
    int socketDescriptor() const { return socketDesc; }
    qint64 writeResponse(int statusCode, THttpResponseHeader &header);
    qint64 writeResponse(int statusCode, THttpResponseHeader &header, const QByteArray &contentType, QIODevice *body, qint64 length);
    qint64 writeResponse(THttpResponseHeader &header, QIODevice *body, qint64 length);

    virtual qint64 writeResponse(THttpResponseHeader &, QIODevice *) { return 0; }
    virtual void closeHttpSocket() { }
    virtual void emitError(int socketError);

    TAtomic<bool> stopped {false};
    QStringList autoRemoveFiles;
    int socketDesc {0};
    TAccessLogger accessLogger;

private:
    TActionController *currController {nullptr};
    QList<TTemporaryFile *> tempFiles;
    THttpRequest *httpReq {nullptr};
    TCache *cachep {nullptr};

    T_DISABLE_COPY(TActionContext)
    T_DISABLE_MOVE(TActionContext)
};

