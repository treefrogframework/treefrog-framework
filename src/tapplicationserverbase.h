#ifndef TAPPLICATIONSERVERBASE_H
#define TAPPLICATIONSERVERBASE_H

#include <QSet>
#include <QMutex>
#include <QHostAddress>
#include <TGlobal>


class T_CORE_EXPORT TApplicationServerBase
{
public:
    enum OpenFlag {
        CloseOnExec = 0,
        NonCloseOnExec,
    };

    virtual ~TApplicationServerBase();
    virtual bool start() { return false; }
    static bool loadLibraries();
    static void nativeSocketInit();
    static void nativeSocketCleanup();
    static int nativeListen(const QHostAddress &address, quint16 port, OpenFlag flag = CloseOnExec);
    static int nativeListen(const QString &fileDomain, OpenFlag flag = CloseOnExec);
    static void nativeClose(int socket);
    static void invokeStaticInitialize();

protected:
    void insertPointer(TActionContext *p);
    void deletePointer(TActionContext *p);
    void releaseAllContexts();
    int actionContextCount() const;

private:
    QSet<TActionContext *> actionContexts;
    mutable QMutex setMutex;

    TApplicationServerBase();
    friend class TThreadApplicationServer;
    friend class TPreforkApplicationServer;
    friend class TMultiplexingServer;
    Q_DISABLE_COPY(TApplicationServerBase)
};

#endif // TAPPLICATIONSERVERBASE_H
