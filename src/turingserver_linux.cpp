/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tkvsdatabasepool.h"
#include "tpublisher.h"
#include "tsqldatabasepool.h"
#include "tsystembus.h"
#include "tsystemglobal.h"
#include "turlroute.h"
#include "turingcoroutine.h"
#include <turingserver.h>
#include <QElapsedTimer>
#include <TActionWorker>
//#include <TAppSettings>
#include <TApplicationServerBase>
#include <TThreadApplicationServer>
#include <TWebApplication>
//#include <sys/eventfd.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <netinet/tcp.h>
//#include <arpa/inet.h>
//#include <unistd.h>
//#include <thread>
//#include <coroutine>
//#include <iostream>
//#include <string>
//#include <functional>
//#include <cstring>
//#include <print>

constexpr int SEND_BUF_SIZE = 8 * 1024;
constexpr int RECV_BUF_SIZE = 16 * 1024;

//namespace {

// TUringServer *uringServer = nullptr;

// void cleanup()
// {
//     delete uringServer;
//     uringServer = nullptr;
// }
// }


// void TUringServer::instantiate(int listeningSocket)
// {
//     if (!uringServer) {
//         uringServer = new TUringServer(listeningSocket);
//         qAddPostRoutine(::cleanup);
//     }
// }


TUringServer *TUringServer::instance(int listeningSocket)
{
    static std::unique_ptr<TUringServer> instance;
    static std::once_flag once;

    std::call_once(once, [&]() {
        if (listeningSocket <= 0) {
            throw StandardException("Invalid socket", __FILE__, __LINE__);
        }
        instance = std::make_unique<TUringServer>(listeningSocket);
    });
    return instance.get();

    // if (Q_UNLIKELY(!uringServer)) {
    //     tFatal("Call TUringServer::instantiate() function first");
    // }
    // return uringServer;
}


static void setNoDeleyOption(int fd)
{
    int res, flag, bufsize;

    // Disable the Nagle (TCP No Delay) algorithm
    flag = 1;
    res = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
    if (res < 0) {
        tSystemWarn("setsockopt error [TCP_NODELAY] fd:{}", fd);
    }

    bufsize = SEND_BUF_SIZE;
    res = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    if (res < 0) {
        tSystemWarn("setsockopt error [SO_SNDBUF] fd:{}", fd);
    }

    bufsize = RECV_BUF_SIZE;
    res = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    if (res < 0) {
        tSystemWarn("setsockopt error [SO_RCVBUF] fd:{}", fd);
    }
}


TUringServer::TUringServer(int listeningSocket, QObject *parent) :
    TDatabaseContextThread(parent),
    TApplicationServerBase(),
    _listenSocket(listeningSocket)
{
    io_uring_queue_init(8192, &_ring, 0);
}


TUringServer::~TUringServer()
{
    io_uring_queue_exit(&_ring);
}


bool TUringServer::start(bool debugMode)
{
    if (isRunning()) {
        return true;
    }

    // Loads libs
    bool res = loadLibraries();
    if (!res) {
        if (debugMode) {
            tSystemError("Failed to load application libraries.");
            return false;
        } else {
            tSystemWarn("Failed to load application libraries.");
        }
    }

    // To work a timer in main thread
    TSqlDatabasePool::instance();
    TKvsDatabasePool::instance();

    TStaticInitializeThread::exec();
    QThread::start();
    return true;
}

/*
int TUringServer::processEvents(int maxMilliSeconds)
{
    TEpoll::instance()->dispatchEvents();

    // Poll Sending/Receiving/Incoming
    maxMilliSeconds = std::max(maxMilliSeconds, 0);
    int res = TEpoll::instance()->wait(maxMilliSeconds);
    if (res < 0) {
        return res;
    }

    TEpollSocket *sock;
    while ((sock = TEpoll::instance()->next())) {

        int cltfd = sock->socketDescriptor();
        if (cltfd == listenSocket && cltfd > 0) {
            if (_processingSocketStack.count() > 64) {  // Not accept in deep context
                continue;
            }

            TEpollSocket *acceptedSock = TEpollHttpSocket::accept(listenSocket);
            if (Q_LIKELY(acceptedSock)) {
                if (!acceptedSock->watch()) {
                    acceptedSock->dispose();
                }
            }
            continue;

        } else {
            if (TEpoll::instance()->canSend()) {
                if (sock->state() == Tf::SocketState::Connecting) {
                    sock->_state = Tf::SocketState::Connected;
                    continue;
                }

                // Send data
                int len = TEpoll::instance()->send(sock);
                if (Q_UNLIKELY(len < 0)) {
                    TEpoll::instance()->deletePoll(sock);
                    sock->dispose();
                    continue;
                }
            }

            if (TEpoll::instance()->canReceive()) {
                try {
                    // Receive data
                    int len = TEpoll::instance()->recv(sock);
                    if (Q_UNLIKELY(len < 0)) {
                        TEpoll::instance()->deletePoll(sock);
                        sock->dispose();
                        continue;
                    }
                } catch (ClientErrorException &e) {
                    Tf::warn("Caught ClientErrorException: status code:{}", e.statusCode());
                    tSystemWarn("Caught ClientErrorException: status code:{}", e.statusCode());
                    TEpoll::instance()->deletePoll(sock);
                    sock->dispose();
                    continue;
                }

                if (sock->canReadRequest()) {
                    _processingSocketStack.push(sock);
                    sock->process();
                    _processingSocketStack.pop();
                }
            }
        }
    }

    // Garbage
    if (!_garbageSockets.isEmpty()) {
        auto set = _garbageSockets;
        for (auto ptr : set) {
            if (!ptr->isProcessing()) {
                delete ptr;  // Remove it from garbageSockets-set in the destructor
            }
        }
    }

    return res;
}
*/


void TUringServer::run()
{
    setNoDeleyOption(_listenSocket);

    // if (newerLibraryExists()) {
        //     tSystemInfo("Detect new library of application. Reloading the libraries.");
        //     Tf::app()->exit(127);
        // }


    //std::cout << "# Event Loop\n";
    //_stop = false;

    TAwaitBase accepter;
    instance()->addAccept(_listenSocket, &accepter);

    struct __kernel_timespec ts {
        .tv_sec = 2,   // 2秒
        .tv_nsec = 0
    };

    // --- イベントループ ---
    while (!_stopped) {
        tSystemDebug("-----------------------------------");
        io_uring_cqe* cqe = nullptr;
        int res = io_uring_wait_cqe_timeout(&_ring, &cqe, &ts);
        Tf::ScopeExitFunction seen([&]{ if (cqe) io_uring_cqe_seen(&_ring, cqe); });

        if (res < 0 || !cqe) {
            if (res == -EINTR || res == -EAGAIN) {
                continue;
            }

            if (res == -ETIME) {
                if (_autoReload && newerLibraryExists()) {
                    tSystemInfo("Detect new library of application. Reloading the libraries.");
                    Tf::app()->exit(127);
                }
                continue;
            }

            tSystemError("io_uring_wait_cqe error: {}", strerror(-res));
            break;
        }

        void* user_data = io_uring_cqe_get_data(cqe);
        if (user_data) {
            auto* await = static_cast<TAwaitBase*>(user_data);
            if (await == &accepter) {
                // マルチショット accept 結果
                if (cqe->res >= 0) {
                    // コルーチンを起動
                    //handleClient(cqe->res);
                    auto routine = std::make_unique<TUringCoroutine>();
                    routine->start(cqe->res);
                    await->clear();  // clear
                } else {
                    int err = -cqe->res;
                    switch (err) {
                    case EAGAIN:
                    case ECONNABORTED:
                    case ECANCELED:
                        // ignore
                        break;
                    case EINVAL:
                    case EBADF:
                    case ENOTSOCK:
                        // listen_fd が閉じられた → 終了処理
                        tSystemError("Listen socket invalid, terminating.  error: {}\n", strerror(err));
                        stop();
                        break;
                    default:
                        tSystemError("Accept error: {}\n", strerror(err));
                        stop();
                        break;
                    }
                }

                if (!(cqe->flags & IORING_CQE_F_MORE)) {
                    //std::print(std::cerr, "flags & IORING_CQE_F_MORE\n");
                    if (addAccept(_listenSocket, &accepter) < 0) {
                        tSystemError("addAccept error: {}", strerror(errno));
                    }
                }
            } else {
                if (!await->_cqeres) {
                    await->_cqeres = cqe->res;
                }
                // recv/send
                //std::print("cqe->res:{}  cqe->flags: {}\n", await->_cqeres, cqe->flags);
                //std::print("#[Loop] resume  res:{}\n", await->_cqeres);
                if (--await->_sqecounter == 0) {
                    if (await->_handle && !await->_handle.done()) {
                        //std::print("[Loop] resume  res:{}\n", await->_cqeres);
                        await->_handle.resume();
                    }
                }
            }
        }
    }
}


void TUringServer::stop()
{
    _stopped = true;
    if (isRunning()) {
        QThread::wait(10000);
    }
    TStaticReleaseThread::exec();
}


void TUringServer::setAutoReloadingEnabled(bool enable)
{
    _autoReload = enable;
    // if (enable) {
    //     reloadTimer.start(500, this);
    // } else {
    //     reloadTimer.stop();
    // }
}


bool TUringServer::isAutoReloadingEnabled()
{
    return _autoReload;
    //return reloadTimer.isActive();
}


TActionContext *TUringServer::currentContext() const
{
    return nullptr;
//    return TURingCoroutine::currentContext();

//     // if (!_processingSocketStack.isEmpty()) {
//     //     auto *worker = _processingSocketStack.top();
//     //     return worker;
//     // }
//     // return nullptr;
}


TActionController *TUringServer::currentController() const
{
    auto *context = currentContext();
    return (context) ? context->currentController() : nullptr;
}

/*
void TUringServer::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != reloadTimer.timerId()) {
        QThread::timerEvent(event);
    } else {
        if (newerLibraryExists()) {
            tSystemInfo("Detect new library of application. Reloading the libraries.");
            Tf::app()->exit(127);
        }
    }
}
*/

// アクセプト
int TUringServer::addAccept(int fd, TAwaitBase* await)
{
    //std::print("addAccept  fd: {}\n", fd);
    io_uring_sqe* sqe = io_uring_get_sqe(&_ring);
    io_uring_prep_multishot_accept(sqe, fd, nullptr, nullptr, (SOCK_CLOEXEC | SOCK_NONBLOCK)); // マルチショット
    if (await) {
        await->clear();
        io_uring_sqe_set_data(sqe, await);
    }
    return io_uring_submit(&_ring);
}

// 受信
int TUringServer::addRecv(int fd, void* buf, size_t len, int msecs, TAwaitBase* await)
{
    //std::print("addRecv  fd: {}\n", fd);
    io_uring_sqe* sqe = io_uring_get_sqe(&_ring);
    io_uring_prep_recv(sqe, fd, buf, len, 0);
    if (await) {
        await->clear();
        io_uring_sqe_set_data(sqe, await);
    }

    if (msecs > 0) {
        sqe->flags |= IOSQE_IO_LINK;
        io_uring_sqe* sqe2 = io_uring_get_sqe(&_ring);
        __kernel_timespec ts;
        ts.tv_sec  = msecs / 1000;
        ts.tv_nsec = (msecs % 1000) * 1'000'000;
        io_uring_prep_link_timeout(sqe2, &ts, 0);
        if (await) {
            await->_sqecounter++;
            io_uring_sqe_set_data(sqe2, await);
        }
    }
    return io_uring_submit(&_ring);
}

int TUringServer::addSend(int fd, const void* buf, size_t len, TAwaitBase* await)
{
    //std::print("addSend  fd: {}\n", fd);
    io_uring_sqe* sqe = io_uring_get_sqe(&_ring);
    io_uring_prep_send_zc(sqe, fd, buf, len, 0, 0);
    if (await) {
        await->clear();
        io_uring_sqe_set_data(sqe, await);
    }
    return io_uring_submit(&_ring);
}

// int addPoll(int fd, int mask, int msecs, TAwaitBase* await)
// {
//     io_uring_sqe* sqe1 = io_uring_get_sqe(&_ring);
//     io_uring_prep_poll_add(sqe1, fd, mask);
//     io_uring_sqe_set_data(sqe1, await);

//     if (msecs > 0) {
//         sqe1->flags |= IOSQE_IO_LINK;
//         io_uring_sqe* sqe2 = io_uring_get_sqe(&_ring);
//         __kernel_timespec ts;
//         ts.tv_sec  = msecs / 1000;
//         ts.tv_nsec = (msecs % 1000) * 1'000'000;
//         io_uring_prep_link_timeout(sqe2, &ts, 0);
//         io_uring_sqe_set_data(sqe2, await);
//         if (await) {
//             await->_sqecounter = 2;
//         }
//     }

//     return io_uring_submit(&_ring);
// }

// 状態変数待ち
int TUringServer::addEvent(int fd, TAwaitBase* await)
{
    io_uring_sqe* sqe = io_uring_get_sqe(&_ring);
    io_uring_prep_poll_add(sqe, fd, POLL_IN);
    if (await) {
        await->clear();
        io_uring_sqe_set_data(sqe, await);
    }
    return io_uring_submit(&_ring);
}
