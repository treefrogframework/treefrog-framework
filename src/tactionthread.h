#ifndef TACTIONTHREAD_H
#define TACTIONTHREAD_H

#include <QThread>
#include <TActionContext>
#include <THttpRequest>

class THttpSocket;
class THttpResponseHeader;
class QIODevice;


class T_CORE_EXPORT TActionThread : public QThread, public TActionContext
{
    Q_OBJECT
public:
    TActionThread(int socket);
    virtual ~TActionThread();

    static bool readRequest(THttpSocket *socket, THttpRequest &request);

protected:
    void run();
    void emitError(int socketError);

    bool readRequest();
    qint64 writeResponse(THttpResponseHeader &header, QIODevice *body);
    void closeHttpSocket();
    void releaseHttpSocket();

signals:
    void error(int socketError);

private:
    THttpSocket *httpSocket;

    Q_DISABLE_COPY(TActionThread)
};

#endif // TACTIONTHREAD_H
