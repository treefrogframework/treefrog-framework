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

    bool start(bool debugMode) override;
    void stop() override;
    bool isSocketOpen() const;
    void setAutoReloadingEnabled(bool enable) override;
    bool isAutoReloadingEnabled() override;

protected:
    void incomingConnection(qintptr socketDescriptor) override;
    void timerEvent(QTimerEvent *event) override;

private:
    int listenSocket;
    int maxThreads;
    QBasicTimer reloadTimer;

    T_DISABLE_COPY(TThreadApplicationServer)
    T_DISABLE_MOVE(TThreadApplicationServer)
};


class TStaticInitializeThread : public TActionThread
{
public:
    static void exec()
    {
        TStaticInitializeThread *initializer = new TStaticInitializeThread();
        initializer->start();
        QThread::yieldCurrentThread();  // needed to avoid deadlock on win
        initializer->wait();
        delete initializer;

    }

protected:
    TStaticInitializeThread() : TActionThread(0) { }

    void run() override
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
        QThread::yieldCurrentThread();
        releaser->wait();
        delete releaser;
    }

protected:
    TStaticReleaseThread() : TActionThread(0) { }

    void run() override
    {
        TApplicationServerBase::invokeStaticRelease();
    }
};

#endif // TTHREADAPPLICATIONSERVER_H
