#ifndef TACTIONCONTEXT_H
#define TACTIONCONTEXT_H

#include <QStringList>
#include <QMap>
#include <TGlobal>
#include <TAccessLog>
#include "tatomic.h"
#include "tdatabasecontext.h"

class QIODevice;
class QHostAddress;
class THttpResponseHeader;
class THttpSocket;
class THttpRequest;
class THttpResponse;
class TApplicationServer;
class TTemporaryFile;
class TActionController;


class T_CORE_EXPORT TActionContext : public TDatabaseContext
{
public:
    TActionContext();
    virtual ~TActionContext();

    TTemporaryFile &createTemporaryFile();
    void stop() { stopped = true; }
    QHostAddress clientAddress() const;
    const TActionController *currentController() const { return currController; }
    THttpRequest &httpRequest() { return *httpReq; }
    const THttpRequest &httpRequest() const { return *httpReq; }

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

    TAtomic<bool> stopped;
    QStringList autoRemoveFiles;
    int socketDesc;
    TAccessLogger accessLogger;

private:
    TActionController *currController;
    QList<TTemporaryFile *> tempFiles;
    THttpRequest *httpReq;

    T_DISABLE_COPY(TActionContext)
    T_DISABLE_MOVE(TActionContext)
};

#endif // TACTIONCONTEXT_H
