#include "turingcoroutine.h"
#include "turingserver.h"
//#include "tactionroutine.h"
#include "TSystemGlobal"
#include "TAppSettings"
#include "THttpRequest"
#include "TTemporaryFile"
#include <QStack>
#include <memory>
#include <cstddef>
#include <sys/eventfd.h>

constexpr uint READ_THRESHOLD_LENGTH = 4 * 1024 * 1024;  // bytes


class AsyncRecv : public TAwaitBase {
public:
    AsyncRecv(int fd, void* buffer, size_t length, int msecs) :
        _fd(fd), _buf(buffer), _len(length), _msecs(msecs) { }

    void await_suspend(std::coroutine_handle<> handle)
    {
        //std::print("== AsyncRecv \n");
        _handle = handle;
        if (TUringServer::instance()->addRecv(_fd, _buf, _len, _msecs, this) < 0) {
            tSystemError("addRecv error: {}", strerror(errno));
        }
    }

    inline int await_resume() { return _cqeres; }

private:
    int _fd {0};
    void* _buf {nullptr};
    size_t _len {0};
    int _msecs {0};
};


class AsyncSend : public TAwaitBase {
public:
    AsyncSend(int fd, const void *buf, size_t len) :
        _fd(fd), _buf(buf), _len(len) { }

    void await_suspend(std::coroutine_handle<> handle)
    {
        //std::print("== AsyncSend \n");
        _handle = handle;
        if (TUringServer::instance()->addSend(_fd, _buf, _len, this) < 0) {
            tSystemError("addSend error: {}", strerror(errno));
        }
    }

    inline int await_resume() { return _cqeres; }

private:
    int _fd {0};
    const void* _buf {nullptr};
    size_t _len {0};
};


template <typename R>
class AsyncFunction : public TAwaitBase {
public:
    explicit AsyncFunction(std::function<R()> f) :
        TAwaitBase(), _func(std::move(f)) {}
    ~AsyncFunction() { if (_fd > 0) ::close(_fd); }

    void await_suspend(std::coroutine_handle<> handle)
    {
        _handle = handle;
        _fd = eventfd(0, (EFD_NONBLOCK | EFD_CLOEXEC));
        TUringServer::instance()->addEvent(_fd, this);
        std::thread([this]() {
            _result = _func();
            notifyResume();
        }).detach();
    }

    R await_resume() { return _result; }

protected:
    void notifyResume()
    {
        if (_fd > 0) {
            uint64_t one = 1;
            write(_fd, &one, sizeof(one)); // 通知
        }
    }

private:
    int _fd {0};
    std::function<R()> _func;
    R _result {};
};


template<typename Func>
class ScopeExitFunction {
public:
    explicit ScopeExitFunction(Func&& func) : _func(std::move(func)) {}
    ~ScopeExitFunction() noexcept { _func(); }
private:
    Func _func;
};


//QStack<TActionContext *> _processingContext;
// QSet<TActionContext*> _activeContexts;

// TActionContext *TUringCoroutine::currentContext()
// {
//     return _activeContexts.
// }


Task TUringCoroutine::start(int sd)
{
    static const int64_t systemLimitBodyBytes = Tf::appSettings()->value(Tf::LimitRequestBody).toLongLong() * 2;
    static int keepAlivetimeout = Tf::appSettings()->value(Tf::HttpKeepAliveTimeout).toInt();

    //_activeContexts.insert(this);
    //ScopeExitFunction remove([&]{ _activeContexts.remove(this); });

    //std::print("accept {}\n", sd);
    ScopeExitFunction closing([sd]{ if (sd > 0) ::close(sd); });
    int res;
    int lengthToRead = -1;

    // ソケット受信
    QByteArray readBuffer;
    readBuffer.reserve(1024);
    QByteArray headerBuffer;
    TTemporaryFile fileBuffer;
tSystemDebug("###1");
    while (lengthToRead) {
        int len = co_await AsyncRecv(sd, readBuffer.data(), 1024, 5000);
tSystemDebug("###2");
        if (len <= 0) {
            if (len < 0) {
tSystemDebug("###3");
                tSystemError("Recv error fd:{} error:{}\n", sd, strerror(-len));
            } else {
                tSystemError("Recv peer closed fd:{}\n", sd);
            }
            co_return;
        }

tSystemDebug("###4");
        readBuffer.resize(readBuffer.size() + len);
        if (lengthToRead < 0) {
tSystemDebug("###5");
            int idx = readBuffer.indexOf(Tf::CRLFCRLF);
            if (idx > 0) {
                THttpRequestHeader header(readBuffer);

                if (systemLimitBodyBytes > 0 && header.contentLength() > systemLimitBodyBytes) {
                    throw ClientErrorException((int)Tf::StatusCode::RequestEntityTooLarge);  // Request Entity Too Large
                }

                lengthToRead = std::max(idx + 4 + header.contentLength() - (int64_t)readBuffer.length(), (int64_t)0);
tSystemDebug("###6 lengthToRead:{}", lengthToRead);

                if (header.contentLength() > READ_THRESHOLD_LENGTH || (header.contentLength() > 0 && header.contentType().trimmed().startsWith("multipart/form-data"))) {
                    headerBuffer = readBuffer.mid(0, idx + 4);
                    // Writes to file buffer
tSystemDebug("###7");
                    if (!fileBuffer.open()) {
                        throw RuntimeException(QLatin1String("temporary file open error: ") + fileBuffer.fileTemplate(), __FILE__, __LINE__);
                    }
                    fileBuffer.resize(0);  // truncate
                    if (readBuffer.length() > idx + 4) {
                        tSystemDebug("fileBuffer name: {}", fileBuffer.fileName());
                        if (fileBuffer.write(readBuffer.data() + idx + 4, readBuffer.length() - (idx + 4)) < 0) {
                            throw RuntimeException(QLatin1String("write error: ") + fileBuffer.fileName(), __FILE__, __LINE__);
                        }
                    }
tSystemDebug("###8");
                    readBuffer.resize(0);
                } else {
tSystemDebug("###9  lengthToRead:{}", lengthToRead);
                    if (lengthToRead > 0) {
                        readBuffer.reserve((idx + 4 + header.contentLength()) * 1.1);
                    }
                }
            } else {
tSystemDebug("###10");
                if (readBuffer.size() > readBuffer.capacity() * 0.8) {
tSystemDebug("###11");
                    readBuffer.reserve(readBuffer.capacity() * 2);
                }
            }
        } else if (lengthToRead > 0) {
tSystemDebug("###12");
            lengthToRead = std::max(lengthToRead - len, 0);
        } else {
tSystemDebug("###13");
            // do nothing
            break;
        }
    }
tSystemDebug("###14  readBuffer: {}", readBuffer);

    //std::string s(buf, buf + res);
    //std::print("[RECV] recv len:{} {}\n", res, s);
    //std::print("[Main] ({}) Received ({} bytes): {}\n", tmp, len, s);


    //auto context = std::make_unique<TActionRoutine>();
    //QList<THttpRequest> THttpRequest::generate(QByteArray &byteArray, const QHostAddress &address, TActionContext *context);
    auto request = THttpRequest::generate(readBuffer, QHostAddress("localhost"), this);

tSystemDebug("###15 : {}", request.header().toByteArray());
tSystemDebug("###15.1 : {}", request.header().cookie("TFSESSION"));
    //THttpRequest request;
    execute(request);
tSystemDebug("###16");

    res = co_await AsyncSend(sd, _response.data(), _response.length());
    if (res <= 0) {
        tSystemError("Send error fd={} res={}\n", sd, res);
        co_return;
    }

    // file
    if (_file.exists())  {
        if (!_file.isOpen()) {
            if (!_file.open(QIODevice::ReadOnly)) {
                Tf::warn("open failed");
                co_return;
            }
        }

        // AsyncSend sender2(sd, _file.data().data(), _file.length());
        // res = co_await sender2;
        // if (res <= 0) {
        //     tSystemError("Send error fd={} res={}\n", sd, res);
        //     co_return;
        // }
    }

    //std::print("send ({}) len:{}\n", tmp, res);
    //std::print("[SEND] fd={} res={}\n", sd, res);
}


// TActionContext *TUringCoroutine::currentContext()
// {
//     if (!_processingContext.isEmpty()) {
//         return _processingContext.top();
//     }
//     return nullptr;
// }


int64_t TUringCoroutine::writeResponse(THttpResponseHeader &header, QIODevice *body)
{
    if (keepAliveTimeout() > 0) {
        header.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("Keep-Alive"));
    }

    // Writes HTTP header
    _response = header.toByteArray();

    if (body) {
        if (body->inherits("QBuffer")) {
            _response += body->readAll();
        } else if (body->inherits("QFile")) {
            //_file = *dynamic_cast<QFile*>(body);

            // TODO TODO TODO TODO TODO TODO TODO TODO
        } else {
            tSystemError("Invalid body [{}:{}]", __FILE__, __LINE__);
            _response += body->readAll();
        }
    }
    return 0;
}
