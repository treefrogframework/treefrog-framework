#ifndef TACTIONTHREAD_H
#define TACTIONTHREAD_H

#include <QThread>
#include <TActionContext>

class THttpSocket;
class THttpRequest;
class THttpRequestHeader;
class THttpResponseHeader;
class QIODevice;


class T_CORE_EXPORT TActionThread : public QThread, public TActionContext
{
    Q_OBJECT
public:
    TActionThread(int socket);
    virtual ~TActionThread();

    static int threadCount();
    static bool waitForAllDone(int msec);
    static QList<THttpRequest> readRequest(THttpSocket *socket);

protected:
    void run() override;
    void emitError(int socketError) override;
    qint64 writeResponse(THttpResponseHeader &header, QIODevice *body) override;
    void closeHttpSocket() override;
    bool handshakeForWebSocket(const THttpRequestHeader &header);

signals:
    void error(int socketError);

private:
    THttpSocket *httpSocket;

    T_DISABLE_COPY(TActionThread)
    T_DISABLE_MOVE(TActionThread)
};

#endif // TACTIONTHREAD_H
