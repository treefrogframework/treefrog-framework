#pragma once
#include <TDatabaseContext>
#include <TAbstractActionContext>
#include <TAtomic>
#include <TAccessLog>
#include <TGlobal>
#include <QMap>
#include <QStringList>

class QIODevice;
class QHostAddress;
class THttpResponseHeader;
class THttpSocket;
class THttpRequest;
class THttpResponse;
class TApplicationServer;
class TTemporaryFile;
class TActionController;


class T_CORE_EXPORT TActionContext : public TDatabaseContext, public TAbstractActionContext {
public:
    TActionContext();
    virtual ~TActionContext();

    TTemporaryFile &createTemporaryFile();
    void stop() { stopped = true; }
    QHostAddress clientAddress() const;
    QHostAddress originatingClientAddress() const;
    const TActionController *currentController() const override { return _currController; }
    TActionController *currentController() { return _currController; }
    const THttpRequest &httpRequest() const override { return *_httpRequest; }
    THttpRequest &httpRequest() override { return *_httpRequest; }
    void flushResponse(TActionController *controller, bool immediate);
    static int keepAliveTimeout();

protected:
    void execute(THttpRequest &request);
    void release();
    qintptr socketDescriptor() const { return socketDesc; }
    int64_t writeResponse(int statusCode, THttpResponseHeader &header);
    int64_t writeResponse(int statusCode, THttpResponseHeader &header, const QByteArray &contentType, QIODevice *body, int64_t length);
    int64_t writeResponse(THttpResponseHeader &header, QIODevice *body, int64_t length);

    virtual int64_t writeResponse(THttpResponseHeader &, QIODevice *) { return 0; }
    virtual void flushSocket() { }
    virtual void closeSocket() { }
    virtual void emitError(int socketError);

    TAtomic<bool> stopped {false};
    QStringList autoRemoveFiles;
    qintptr socketDesc {0};
    TAccessLogger accessLogger;

private:
    TActionController *_currController {nullptr};
    QList<TTemporaryFile *> _tempFiles;
    THttpRequest *_httpRequest {nullptr};

    T_DISABLE_COPY(TActionContext)
    T_DISABLE_MOVE(TActionContext)
};

