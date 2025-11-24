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
#include <TApplicationServerBase>
#include <TThreadApplicationServer>
#include <TWebApplication>
#include <netinet/tcp.h>

constexpr int SEND_BUF_SIZE = 128 * 1024;
constexpr int RECV_BUF_SIZE = 128 * 1024;
constexpr int SPLICE_LEN = 1024 * 1024;


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
}


static void setDefferAcceptOption(int fd)
{
    int res, flag;

    flag = 2;  // secs
    res = setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, (char *)&flag, sizeof(flag));
    if (res < 0) {
        tSystemWarn("setsockopt error [TCP_DEFER_ACCEPT] fd:{}", fd);
    }
}


static void setBufferOption(int fd)
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


void TUringServer::run()
{
    setDefferAcceptOption(_listenSocket);

    TAwaitBase accepter;
    instance()->addAccept(_listenSocket, &accepter);

    __kernel_timespec ts = {
        .tv_sec = 2,   // 2秒
        .tv_nsec = 0
    };

    // --- イベントループ ---
    while (!_stopped) {
        if (_garbage.size() > 0) {
            // Garbage collection
            for (auto it = _garbage.begin(); it != _garbage.end(); ++it) {
                delete *it;
            }
            _garbage.clear();
        }

        io_uring_cqe *cqe = nullptr;
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

        void *user_data = io_uring_cqe_get_data(cqe);
        if (user_data) {
            auto *await = static_cast<TAwaitBase*>(user_data);
            if (await == &accepter) {
                // Accepts
                if (cqe->res >= 0) {
                    // Starts coroutine
                    int fd = cqe->res;
                    setBufferOption(fd);
                    auto *coro = new TUringCoroutine(fd);
                    Task task = coro->start();
                    task.handle.promise().self = coro;
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
                        tSystemError("Listen socket invalid, terminating.  error: {}", strerror(err));
                        stop();
                        break;
                    default:
                        tSystemError("Accept error: {}\n", strerror(err));
                        stop();
                        break;
                    }
                }

                if (!(cqe->flags & IORING_CQE_F_MORE)) {
                    if (addAccept(_listenSocket, &accepter) < 0) {
                        tSystemError("addAccept error: {}", strerror(errno));
                    }
                }
            } else {
                tSystemDebug("cqe->res:{}  cqe->flags: {}", await->_cqeres, cqe->flags);
                if (cqe->flags != IORING_CQE_F_MORE) {
                    if (!await->_cqeres && !await->_cqeflags) {
                        await->_cqeres = cqe->res;
                        await->_cqeflags = cqe->flags;
                    }

                    if (--await->_sqecounter == 0 && await->_handle && !await->_handle.done()) {
                        _currentCoroutine = await->_handle.promise().self;
                        await->_handle.resume();
                        _currentCoroutine = nullptr;
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
}


bool TUringServer::isAutoReloadingEnabled()
{
    return _autoReload;
}


TActionContext *TUringServer::currentContext() const
{
    return _currentCoroutine;
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

//
// Prepare a accept request
//
int TUringServer::addAccept(int fd, TAwaitBase* await) const
{
    io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    if (!sqe) {
        tSystemError("io_uring_get_sqe error: {} [{}:{}]", strerror(errno), __FILE__, __LINE__);
        return -1;
    }

    io_uring_prep_accept(sqe, fd, nullptr, nullptr, (SOCK_CLOEXEC | SOCK_NONBLOCK));
    if (await) {
        await->clear();
        io_uring_sqe_set_data(sqe, await);
    }
    return io_uring_submit(&_ring);
}

//
// Prepare a recv request
//
int TUringServer::addRecv(int fd, void* buf, size_t len, int msecs, TAwaitBase* await) const
{
    io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    if (!sqe) {
        tSystemError("io_uring_get_sqe error: {} [{}:{}]", strerror(errno), __FILE__, __LINE__);
        return -1;
    }

    io_uring_prep_recv(sqe, fd, buf, len, 0);
    if (await) {
        await->clear();
        io_uring_sqe_set_data(sqe, await);
    }

    if (msecs > 0) {
        sqe->flags |= IOSQE_IO_LINK;
        io_uring_sqe *sqe2 = io_uring_get_sqe(&_ring);
        if (!sqe2) {
            tSystemError("io_uring_get_sqe error: {} [{}:{}]", strerror(errno), __FILE__, __LINE__);
            return -1;
        }
        __kernel_timespec ts = { .tv_sec  = msecs / 1000, .tv_nsec = (msecs % 1000) * 1'000'000 };
        io_uring_prep_link_timeout(sqe2, &ts, 0);
        if (await) {
            await->_sqecounter++;
            io_uring_sqe_set_data(sqe2, await);
        }
    }
    return io_uring_submit(&_ring);
}

//
// Prepare a send request
//
int TUringServer::addSend(int fd, const void* buf, size_t len, TAwaitBase* await) const
{
    io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    if (!sqe) {
        tSystemError("io_uring_get_sqe error: {} [{}:{}]", strerror(errno), __FILE__, __LINE__);
        return -1;
    }
    io_uring_prep_send(sqe, fd, buf, len, 0);
    if (await) {
        await->clear();
        io_uring_sqe_set_data(sqe, await);
    }
    return io_uring_submit(&_ring);
}

//
// Prepare a zerocopy send request
//
int TUringServer::addSendZc(int fd, const void* buf, size_t len, TAwaitBase* await) const
{
    io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    if (!sqe) {
        tSystemError("io_uring_get_sqe error: {} [{}:{}]", strerror(errno), __FILE__, __LINE__);
        return -1;
    }
    io_uring_prep_send_zc(sqe, fd, buf, len, 0, 0);
    if (await) {
        await->clear();
        io_uring_sqe_set_data(sqe, await);
    }
    return io_uring_submit(&_ring);
}

// int TUringServer::addSendFile(int sd, int fd, int offset, size_t slice_len, TAwaitBase* await)
// {
//     size_t file_size = lseek(fd, 0, SEEK_END);
//     if (file_size < 0) {
//         tSystemError("lseek error: {}\n", strerror(err));
//         return -1;
//     }
//     lseek(fd, 0, SEEK_SET);

//     void *mapped = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
//     if (mapped == MAP_FAILED) {
//         tSystemError("mmap error: {}\n", strerror(err));
//         return -1;
//     }

//     return addSend(sd, const void* bu, size_t slice_len, await);
// }


//
// Prepare a event request
//
int TUringServer::addEvent(int fd, TAwaitBase* await) const
{
    io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    io_uring_prep_poll_add(sqe, fd, POLL_IN);
    if (await) {
        await->clear();
        io_uring_sqe_set_data(sqe, await);
    }
    return io_uring_submit(&_ring);
}


void TUringServer::registerForGC(TUringCoroutine *coroutine)
{
    _garbage.push_back(coroutine);
}
