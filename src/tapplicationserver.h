#ifndef TAPPLICATIONSERVER_H
#define TAPPLICATIONSERVER_H

#include <QTcpServer>
#include <QSet>
#include <QMutex>
#include <TGlobal>

class TActionContext;


class T_CORE_EXPORT TApplicationServer : public QTcpServer
{
    Q_OBJECT
public:
    enum OpenFlag {
        CloseOnExec = 0,
        NonCloseOnExec,
    };

    TApplicationServer(QObject *parent = 0);
    ~TApplicationServer();

    bool open();
    bool isOpen() const;

    static bool loadLibraries();
    static void nativeSocketInit();
    static void nativeSocketCleanup();
    static int nativeListen(const QHostAddress &address, quint16 port, OpenFlag flag = CloseOnExec);
    static int nativeListen(const QString &fileDomain, OpenFlag flag = CloseOnExec);
    static void nativeClose(int socket);

public slots:
    void close();
    void terminate();

protected:
#if QT_VERSION >= 0x050000
    void incomingConnection(qintptr socketDescriptor);
#else
    void incomingConnection(int socketDescriptor);
#endif
    void insertPointer(TActionContext *p);
    int actionContextCount() const;

protected slots:
    void deleteActionContext();

private:
    int maxServers;
    QSet<TActionContext *> actionContexts;
    mutable QMutex setMutex;

    Q_DISABLE_COPY(TApplicationServer)
};

#endif // TAPPLICATIONSERVER_H
