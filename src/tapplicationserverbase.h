#pragma once
#include <QHostAddress>
#include <TGlobal>


class T_CORE_EXPORT TApplicationServerBase {
public:
    enum OpenFlag {
        CloseOnExec = 0,
        NonCloseOnExec,
    };

    virtual ~TApplicationServerBase();
    virtual bool start(bool) { return false; }
    virtual void stop() { }
    virtual void setAutoReloadingEnabled(bool) { }
    virtual bool isAutoReloadingEnabled() { return false; }

    static bool loadLibraries();
    static void unloadLibraries();
    static QDateTime latestLibraryTimestamp();
    static bool newerLibraryExists();
    static void nativeSocketInit();
    static void nativeSocketCleanup();
    static int nativeListen(const QHostAddress &address, quint16 port, OpenFlag flag = CloseOnExec);
    static int nativeListen(const QString &fileDomain, OpenFlag flag = CloseOnExec);
    static void nativeClose(int socket);
    static QPair<QHostAddress, quint16> getPeerInfo(int socketDescriptor);
    static int duplicateSocket(int socketDescriptor);
    static void invokeStaticInitialize();
    static void invokeStaticRelease();

private:
    TApplicationServerBase();

    friend class TThreadApplicationServer;
    friend class TMultiplexingServer;
    T_DISABLE_COPY(TApplicationServerBase)
    T_DISABLE_MOVE(TApplicationServerBase)
};

