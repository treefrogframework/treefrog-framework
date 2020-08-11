#pragma once
#include "tstack.h"
#include <QBasicTimer>
#include <QTcpServer>
#include <QtGlobal>
#include <TActionThread>
#include <TApplicationServerBase>
#include <TGlobal>


#if defined(Q_OS_WIN) || defined(Q_OS_DARWIN)
class T_CORE_EXPORT TThreadApplicationServer : public QTcpServer, public TApplicationServerBase {
    Q_OBJECT
public:
    TThreadApplicationServer(int listeningSocket, QObject *parent = 0);
    ~TThreadApplicationServer() { }

    bool start(bool debugMode) override;
    void stop() override;
    void setAutoReloadingEnabled(bool enable) override;
    bool isAutoReloadingEnabled() override;

protected:
    void incomingConnection(qintptr socketDescriptor) override;
    void timerEvent(QTimerEvent *event) override;

private:
    static TStack<TActionThread *> *threadPoolPtr();

    int listenSocket {0};
    int maxThreads {0};
    QBasicTimer reloadTimer;

    T_DISABLE_COPY(TThreadApplicationServer)
    T_DISABLE_MOVE(TThreadApplicationServer)
};
#else
class T_CORE_EXPORT TThreadApplicationServer : public QThread, public TApplicationServerBase {
    Q_OBJECT
public:
    TThreadApplicationServer(int listeningSocket, QObject *parent = 0);
    ~TThreadApplicationServer() { }

    bool start(bool debugMode) override;
    void stop() override;
    void setAutoReloadingEnabled(bool enable) override;
    bool isAutoReloadingEnabled() override;

protected:
    void incomingConnection(qintptr socketDescriptor);
    void timerEvent(QTimerEvent *event) override;
    void run() override;

private:
    static TStack<TActionThread *> *threadPoolPtr();

    int listenSocket {0};
    int maxThreads {0};
    QBasicTimer reloadTimer;
    bool stopFlag {false};

    T_DISABLE_COPY(TThreadApplicationServer)
    T_DISABLE_MOVE(TThreadApplicationServer)
};
#endif


class TStaticInitializeThread : public TActionThread {
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
    TStaticInitializeThread() :
        TActionThread(0) { }

    void run() override
    {
        TApplicationServerBase::invokeStaticInitialize();
        commitTransactions();
    }
};


class TStaticReleaseThread : public TActionThread {
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
    TStaticReleaseThread() :
        TActionThread(0) { }

    void run() override
    {
        TApplicationServerBase::invokeStaticRelease();
        commitTransactions();
    }
};

