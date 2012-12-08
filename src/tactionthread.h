#ifndef TACTIONTHREAD_H
#define TACTIONTHREAD_H

#include <QThread>
#include <TActionContext>


class T_CORE_EXPORT TActionThread : public QThread, public TActionContext
{
    Q_OBJECT
public:
    TActionThread(int socket);
    virtual ~TActionThread();

protected:
    virtual void run();
    virtual void emitError(int socketError);

signals:
    void error(int socketError);

private:
    Q_DISABLE_COPY(TActionThread)
};

#endif // TACTIONTHREAD_H
