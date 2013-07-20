#ifndef TACTIONWORKER_H
#define TACTIONWORKER_H

#include <QThread>
#include <TActionContext>

class THttpRequest;
class THttpResponseHeader;
class QIODevice;


class T_CORE_EXPORT TActionWorker : public QThread, public TActionContext
{
    Q_OBJECT
public:
    TActionWorker(int socket, const THttpRequest &request);
    virtual ~TActionWorker();

protected:
    void run() { TActionContext::execute(); }
    bool readRequest() { return true; }
    qint64 writeResponse(THttpResponseHeader &header, QIODevice *body);
    void closeHttpSocket();
    void releaseHttpSocket() { }

private:
    Q_DISABLE_COPY(TActionWorker)
};

#endif // TACTIONWORKER_H
