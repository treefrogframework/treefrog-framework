#ifndef TACTIONPROCESS_H
#define TACTIONPROCESS_H

#include <QObject>
#include <QBasicTimer>
#include <QMap>
#include <TGlobal>
#include <TActionContext>


class T_CORE_EXPORT TActionProcess : public QObject, public TActionContext
{
    Q_OBJECT
public:
    TActionProcess(int socket);
    virtual ~TActionProcess();

    void start();
    static TActionProcess *currentProcess();
    static bool isChildProcess();

protected:
    enum ProcessType {
        ParentProcess = 0,
        ChildProcess,
        Neither,
    };

    virtual bool openDatabase();
    virtual void closeDatabase();
    virtual void emitError(int socketError);

protected slots:
    void terminate(int status);
    void kill(int sig);

signals:
    void started();  // for the parent
    void forked();   // for the child
    void finished(); // for the parent and the child
    void error(int socketError);

private:
    int childPid;
    static TActionProcess *currentActionProcess;

    friend class TActionProcessManager;
    Q_DISABLE_COPY(TActionProcess)
};


/**
 * TActionProcessManager class
 */
class TActionProcessManager : public QObject, public QMap<int, TActionProcess *>
{
public:
    TActionProcessManager();
    ~TActionProcessManager();
    static TActionProcessManager *instance();
    
protected:
    void timerEvent(QTimerEvent *event);

private:
    QBasicTimer timer;
    Q_DISABLE_COPY(TActionProcessManager)
};

#endif // TACTIONPROCESS_H
