#ifndef TAPPLICATIONSERVERBASE_H
#define TAPPLICATIONSERVERBASE_H

#include <QSet>
#include <QMutex>
#include <QHostAddress>
#include <TGlobal>

#ifdef Q_OS_UNIX
# include <tfcore_unix.h>
#endif


class T_CORE_EXPORT TApplicationServerBase
{
public:
    enum OpenFlag {
        CloseOnExec = 0,
        NonCloseOnExec,
    };

    virtual ~TApplicationServerBase();
    virtual bool start() { return false; }
    virtual void stop() { }
    virtual void setAutoReloadingEnabled(bool) { }
    virtual bool isAutoReloadingEnabled() { return false; }

    static bool loadLibraries();
    static QDateTime latestLibraryTimestamp();
    static bool newerLibraryExists();
    static void nativeSocketInit();
    static void nativeSocketCleanup();
    static int nativeListen(const QHostAddress &address, quint16 port, OpenFlag flag = CloseOnExec);
    static int nativeListen(const QString &fileDomain, OpenFlag flag = CloseOnExec);
    static void nativeClose(int socket);
    static int duplicateSocket(int socketDescriptor);
    static void invokeStaticInitialize();
    static void invokeStaticRelease();

private:
    TApplicationServerBase();

    friend class TThreadApplicationServer;
    friend class TPreforkApplicationServer;
    friend class TMultiplexingServer;
    Q_DISABLE_COPY(TApplicationServerBase)
};

#endif // TAPPLICATIONSERVERBASE_H
