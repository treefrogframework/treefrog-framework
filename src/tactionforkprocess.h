#ifndef TACTIONFORKPROCESS_H
#define TACTIONFORKPROCESS_H

#include <QObject>
#include <TActionContext>


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

    static TActionForkProcess *currentActionContext;

signals:
    void finished();
    void error(int socketError);

private:
    Q_DISABLE_COPY(TActionForkProcess)
};

#endif // TACTIONFORKPROCESS_H
