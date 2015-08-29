#ifndef TTHREADAPPLICATIONSERVER_H
#define TTHREADAPPLICATIONSERVER_H

#include <QTcpServer>
#include <QBasicTimer>
#include <TGlobal>
#include <TApplicationServerBase>
#include <TActionThread>


class T_CORE_EXPORT TThreadApplicationServer : public QTcpServer, public TApplicationServerBase
{
    Q_OBJECT
public:
    TThreadApplicationServer(int listeningSocket, QObject *parent = 0);
    ~TThreadApplicationServer();

    bool start();
    void stop();
    bool isSocketOpen() const;
    void setAutoReloadingEnabled(bool enable);
    bool isAutoReloadingEnabled();

protected:
#if QT_VERSION >= 0x050000
    void incomingConnection(qintptr socketDescriptor);
#else
    void incomingConnection(int socketDescriptor);
#endif
    void timerEvent(QTimerEvent *event);

private:
    int listenSocket;
    int maxThreads;
    QBasicTimer reloadTimer;

    Q_DISABLE_COPY(TThreadApplicationServer)
};


class TStaticInitializeThread : public TActionThread
{
public:
    static void exec()
    {
        TStaticInitializeThread *initializer = new TStaticInitializeThread();
        initializer->start();
        initializer->wait();
        delete initializer;

    }

protected:
    TStaticInitializeThread() : TActionThread(0) { }

    void run()
    {
        TApplicationServerBase::invokeStaticInitialize();
    }
};


class TStaticReleaseThread : public TActionThread
{
public:
    static void exec()
    {
        TStaticReleaseThread *releaser = new TStaticReleaseThread();
        releaser->start();
        releaser->wait();
        delete releaser;
    }

protected:
    TStaticReleaseThread() : TActionThread(0) { }

    void run()
    {
        TApplicationServerBase::invokeStaticRelease();
    }
};

#endif // TTHREADAPPLICATIONSERVER_H
