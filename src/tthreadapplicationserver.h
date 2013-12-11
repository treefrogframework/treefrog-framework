#ifndef TTHREADAPPLICATIONSERVER_H
#define TTHREADAPPLICATIONSERVER_H

#include <QTcpServer>
#include <TGlobal>
#include <TApplicationServerBase>
#include <TActionThread>


class T_CORE_EXPORT TThreadApplicationServer : public QTcpServer, public TApplicationServerBase
{
    Q_OBJECT
public:
    TThreadApplicationServer(QObject *parent = 0);
    ~TThreadApplicationServer();

    bool start();
    void stop();
    bool isSocketOpen() const;

public slots:
    void terminate();

protected:
#if QT_VERSION >= 0x050000
    void incomingConnection(qintptr socketDescriptor);
#else
    void incomingConnection(int socketDescriptor);
#endif

private:
    int maxServers;

    Q_DISABLE_COPY(TThreadApplicationServer)
};


class TStaticInitializeThread : public TActionThread
{
public:
    TStaticInitializeThread() : TActionThread(0) { }
protected:
    void run()
    {
        TApplicationServerBase::invokeStaticInitialize();
    }
};

#endif // TTHREADAPPLICATIONSERVER_H
