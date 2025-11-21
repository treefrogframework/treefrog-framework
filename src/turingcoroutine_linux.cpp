#include "turingcoroutine.h"
#include "turingserver.h"
#include "TSystemGlobal"
#include "TAppSettings"
#include "THttpRequest"
#include "TTemporaryFile"
#include <QStack>
#include <memory>
#include <cstddef>
#include <sys/eventfd.h>
#include <sys/mman.h>

constexpr uint READ_THRESHOLD_LENGTH = 4 * 1024 * 1024;  // bytes


class AsyncRecv : public TAwaitBase {
public:
    AsyncRecv(int fd, void* buffer, size_t length, int msecs) :
        _fd(fd), _buf(buffer), _len(length), _msecs(msecs) { }

    void await_suspend(std::coroutine_handle<Task::promise_type> handle)
    {
        _handle = handle;
        if (TUringServer::instance()->addRecv(_fd, _buf, _len, _msecs, this) < 0) {
            tSystemError("addRecv error: {}", strerror(errno));
        }
    }

    inline int await_resume()
    {
        //tSystemDebug("await_resume : _len:{} _cqeflags:{} _cqeres:{}", _len, _cqeflags, _cqeres);
        return _cqeres;
    }

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

    void await_suspend(std::coroutine_handle<Task::promise_type> handle)
    {
        _handle = handle;
        int res = TUringServer::instance()->addSendZc(_fd, _buf, _len, this);
        // res = TUringServer::instance()->addSend(_fd, _buf, _len, this);

        if (res < 0) {
            tSystemError("addSend error: {}", strerror(errno));
        }
    }

    inline int await_resume()
    {
        tSystemDebug("await_resume : _len:{} _cqeflags:{} _cqeres:{}", _len, _cqeflags, _cqeres);
        return (_cqeflags == IORING_CQE_F_NOTIF) ? _len : _cqeres;
    }

private:
    int _fd {0};
    const void* _buf {nullptr};
    size_t _len {0};
};


template <typename R>
class AsyncFunction : public TAwaitBase {
public:
    explicit AsyncFunction(std::function<R()> f, TUringCoroutine *parent) :
        TAwaitBase(parent), _func(std::move(f)) {}
    ~AsyncFunction() { if (_fd > 0) ::close(_fd); }

    void await_suspend(std::coroutine_handle<Task::promise_type> handle)
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


class CurrentRoutineScope {
public:
    CurrentRoutineScope(TUringCoroutine *&slot, TUringCoroutine *now) :
        _slot(slot), _prev(slot)
    {
        _slot = now;
    }
    ~CurrentRoutineScope() { _slot = _prev; }
private:
    TUringCoroutine *&_slot;
    TUringCoroutine *_prev;
};


TUringCoroutine::~TUringCoroutine()
{
    tSystemDebug("~TUringCoroutine: sd:{}", _sd);
    if (_sd > 0) {
        ::close(_sd);
    }
}


Task TUringCoroutine::start()
{
    static const int64_t systemLimitBodyBytes = Tf::appSettings()->value(Tf::LimitRequestBody).toLongLong() * 2;
    static int keepAlivetimeout = Tf::appSettings()->value(Tf::HttpKeepAliveTimeout).toInt();
    constexpr int64_t bufsize = 8 * 1024;

    ScopeExitFunction closing([this]{
        TUringServer::instance()->registerForGC(this);
    });

    int timeout = 5000;
    while (timeout > 0) {
        //int res;
        int64_t lengthToRead = INT64_MAX;
        int64_t readLength = 0;

        // ソケット受信
        QByteArray readBuffer;
        QByteArray headerBuffer;
        TTemporaryFile fileBuffer;

        while (lengthToRead > 0) {
            int64_t buflen = std::min(bufsize, lengthToRead);
            readBuffer.reserve(readLength + buflen);

            int len = co_await AsyncRecv(_sd, readBuffer.data() + readLength, buflen, timeout);
            if (len < 0) {
                // timeout or error
                if (len == -ETIME) {
                    tSystemDebug("Recv timer expired fd:{}", _sd);
                } else {
                    tSystemError("Recv error fd:{} error:{}", _sd, strerror(-len));
                }
                co_return;
            }
            if (!len) {
                tSystemWarn("Recv peer closed fd:{}", _sd);
                co_return;
            }

            readLength += len;
            readBuffer.resize(readLength);
            tSystemDebug("readBuffer size:{}", readBuffer.size());

            int idx = readBuffer.indexOf(Tf::CRLFCRLF);
            if (idx > 0) {
                THttpRequestHeader header(readBuffer);

                if (systemLimitBodyBytes > 0 && header.contentLength() > systemLimitBodyBytes) {
                    throw ClientErrorException((int)Tf::StatusCode::RequestEntityTooLarge);  // Request Entity Too Large
                }

                lengthToRead = std::max(idx + 4 + header.contentLength() - (int64_t)readBuffer.length(), (int64_t)0);

                if (header.contentLength() > READ_THRESHOLD_LENGTH || (header.contentLength() > 0 && header.contentType().trimmed().startsWith("multipart/form-data"))) {
                    headerBuffer = readBuffer.mid(0, idx + 4);
                    // Writes to file buffer
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
                    readBuffer.resize(0);
                }
            }
        }

        auto request = THttpRequest::generate(readBuffer, QHostAddress("localhost"), this);
        execute(request);

        int res = co_await AsyncSend(_sd, _response.data(), _response.length());
        if (res <= 0) {
            tSystemError("Send error fd={} res={}\n", _sd, res);
            co_return;
        }

        // file
        if (!_fileName.isEmpty()) {
            //int64_t fileSize = QFileInfo(_fileName).size();
            int fd = ::open(qUtf8Printable(_fileName), O_RDONLY | O_CLOEXEC);
            if (fd < 0) {
                tSystemDebug("File open error: {}", _fileName);
                Tf::warn("File open error: {}", _fileName);
                co_return;
            }

            ScopeExitFunction fd_closing([fd]{ ::close(fd); });

            const int64_t file_size = lseek(fd, 0, SEEK_END);
            if (file_size < 0) {
                tSystemDebug("lseek error: {}", strerror(errno));
                co_return;
            }
            lseek(fd, 0, SEEK_SET);

            void *mapped = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
            if (mapped == MAP_FAILED) {
                tSystemError("mmap error: {}\n", strerror(errno));
                co_return;
            }

            const int64_t slice_len = 16 * 1024;
            int64_t sent_len = 0;
            while (sent_len < file_size) {
                int64_t send_len = std::min(file_size - sent_len, slice_len);
                res = co_await AsyncSend(_sd, (char*)mapped + sent_len, send_len);
                if (res <= 0) {
                    if (res == -EAGAIN || res == -ENOMEM) {
                        continue;
                    }
                    tSystemError("Send error fd={} res={}", _sd, res);
                    break;
                }
                sent_len += res;
                //tSystemDebug("### AsyncSend  res:{}  sent_len:{}", res, sent_len);
            }

            munmap(mapped, file_size);
            _fileName.resize(0);
        }

        if (keepAlivetimeout > 0) {
            timeout = keepAlivetimeout * 1000;  // msecs
        } else {
            break;
        }
    }
}


int64_t TUringCoroutine::writeResponse(THttpResponseHeader &header, QIODevice *body)
{
    if (keepAliveTimeout() > 0) {
        header.setRawHeader(QByteArrayLiteral("Connection"), QByteArrayLiteral("Keep-Alive"));
    }
    // Writes HTTP header
    _response = header.toByteArray();

    if (body) {
        if (auto *buf = dynamic_cast<QBuffer*>(body); buf) {
            _response += buf->buffer();
        } else if (auto *file = dynamic_cast<QFile*>(body); file) {
            _fileName = file->fileName();
        } else {
            tSystemError("Invalid body [{}:{}]", __FILE__, __LINE__);
        }
    }
    return 0;
}
