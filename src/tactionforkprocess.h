#ifndef TACTIONFORKPROCESS_H
#define TACTIONFORKPROCESS_H

#include <QObject>
#include <TActionContext>

class THttpSocket;


class T_CORE_EXPORT TActionForkProcess : public QObject, public TActionContext
{
    Q_OBJECT
public:
    TActionForkProcess(int socket);
    virtual ~TActionForkProcess();

    void start();
    static TActionForkProcess *currentContext();

protected:
    virtual void emitError(int socketError);
    virtual qint64 writeResponse(THttpResponseHeader &header, QIODevice *body);
    virtual void closeHttpSocket();

    static TActionForkProcess *currentActionContext;

signals:
    void finished();
    void error(int socketError);

private:
    THttpSocket *httpSocket;

    Q_DISABLE_COPY(TActionForkProcess)
};

#endif // TACTIONFORKPROCESS_H
