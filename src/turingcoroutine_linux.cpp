#include "turingcoroutine.h"
#include "turingserver.h"
#include "tthreadpoolawaiter.h"
#include "tactioncontextroutine.h"
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
    AsyncRecv(int sd, void* buffer, size_t length, int msecs) :
        _sd(sd), _buf(buffer), _len(length), _msecs(msecs) { }

    void await_suspend(std::coroutine_handle<TUringTask::promise_type> handle)
    {
        _handle = handle;
        if (TUringServer::instance()->addRecv(_sd, _buf, _len, _msecs, this) < 0) {
            tSystemError("addRecv error: {}", strerror(errno));
        }
    }

    inline int await_resume()
    {
        return _cqeres;
    }

private:
    int _sd {0};
    void* _buf {nullptr};
    size_t _len {0};
    int _msecs {0};
};


class AsyncSend : public TAwaitBase {
public:
    AsyncSend(int sd, const void *buf, size_t len) :
        _sd(sd), _buf(buf), _len(len) { }

    void await_suspend(std::coroutine_handle<TUringTask::promise_type> handle)
    {
        _handle = handle;
        int res = TUringServer::instance()->addSendZc(_sd, _buf, _len, this);

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
    int _sd {0};
    const void* _buf {nullptr};
    size_t _len {0};
};


class AsyncSendFile : public TAwaitBase {
public:
    AsyncSendFile(int sd, int fd, int fileSize) :
        _sd(sd), _fd(fd), _fileSize(fileSize)
    {
        if (::pipe2(_pipefd, O_CLOEXEC) < 0) {
            tSystemError("pipe error: {}", strerror(errno));
        }
    }

    ~AsyncSendFile()
    {
        if (_pipefd[0] > 0) {
            tf_close(_pipefd[0]);
        }

        if (_pipefd[1] > 0) {
            tf_close(_pipefd[1]);
        }
    }

    void await_suspend(std::coroutine_handle<TUringTask::promise_type> handle)
    {
        _handle = handle;
        iterate();
    }

    inline int await_resume()
    {
        //tSystemInfo("AsyncSendFile::await_resume : _offset:{} _cqeflags:{} _cqeres:{}", _offset, _cqeflags, _cqeres);
        return (_cqeres < 0) ? _cqeres : _offset;
    }

    bool completed() const override
    {
        //tSystemInfo("AsyncSendFile::completed : _offset:{} _cqeflags:{} _cqeres:{}", _offset, _cqeflags, _cqeres);
        switch (_state) {
        case State::WaitForPollOut:
            if (_cqeres == POLLOUT) {
                return (_offset + _cqeres >= _fileSize);
            } else {
                // POLLERR or POLLHUP
                return true;
            }
            break;

        case State::Sending:
            return (_cqeres <= 0);

        case State::Idle:
        default:
            tSystemError("Bad status  [{}:{}]", __FILE__, __LINE__);
            return true;
        }
    }

    void iterate() override
    {
        //tSystemInfo("AsyncSendFile::iterate : _offset:{}  state:{}", _offset, (int)_state);
        constexpr size_t SPLICE_LEN = 256 * 1024;

        switch (_state) {
        case State::WaitForPollOut:
            if (_cqeres < 0) {
                return;
            }
            _state = State::Idle;
            _cqeres = 0;
            [[fallthrough]];

        case State::Idle: {
            size_t len = std::min<size_t>(SPLICE_LEN, _fileSize - _offset);
            int res = TUringServer::instance()->addSendFile(_sd, _fd, _offset, len, _pipefd, this);
            if (res < 0) {
                tSystemError("addSend error: {}", strerror(errno));
                _cqeres = -1;
            } else {
                _state = State::Sending;
            }
            break; }

        case State::Sending: {
            if (_cqeres > 0) {
                _offset += _cqeres;
                _cqeres = 0;

                if (_offset >= _fileSize) {
                    return;
                }
            }

            int res = TUringServer::instance()->addPoll(_sd, (POLLOUT | POLLERR | POLLHUP), this);
            if (res < 0) {
                tSystemError("addPoll error: {}", strerror(errno));
                _cqeres = -1;
            } else {
                _state = State::WaitForPollOut;
            }
            break; }

        default:
            break;
        }
    }

private:
    enum class State {
        Idle,
        Sending,
        WaitForPollOut,
    };

    int _sd {0};
    int _fd {0};
    size_t _fileSize {0};
    size_t _offset {0};
    int _pipefd[2] {0};
    State _state {State::Idle};
};


template<typename Func>
class ScopeExitFunction {
public:
    explicit ScopeExitFunction(Func&& func) : _func(std::move(func)) {}
    ~ScopeExitFunction() noexcept { _func(); }
private:
    Func _func;
};


TUringCoroutine::~TUringCoroutine()
{
    //tSystemDebug("~TUringCoroutine: sd:{}", _sd);
    if (_sd > 0) {
        tf_close(_sd);
    }
}


TUringTask TUringCoroutine::start()
{
    static const int64_t systemLimitBodyBytes = Tf::appSettings()->value(Tf::LimitRequestBody).toLongLong();
    static int keepAlivetimeout = Tf::appSettings()->value(Tf::HttpKeepAliveTimeout).toInt();
    constexpr int64_t bufsize = 8 * 1024;

    ScopeExitFunction closing([this]{
        TUringServer::instance()->registerForGC(this);
    });

    TActionContextRoutine routine;
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

        auto result = co_await TThreadPoolAwaiter([&] {
            routine.start(readBuffer);
            return routine.result;
        });
        _response = std::move(result.response);
        _fileName = std::move(result.fileName);

        if (!_response.isEmpty()) {
            int res = co_await AsyncSend(_sd, _response.data(), _response.length());
            if (res <= 0) {
                tSystemError("Send error fd={} res={}", _sd, res);
                co_return;
            }
        }

        // File
        if (!_fileName.isEmpty()) {
            int fd = ::open(qUtf8Printable(_fileName), O_RDONLY | O_CLOEXEC);
            if (fd < 0) {
                tSystemError("File open error: {}", _fileName);
                Tf::warn("File open error: {}", _fileName);
                co_return;
            }

            ScopeExitFunction fd_closing([fd]{ ::close(fd); });

            struct stat st{};
            if (fstat(fd, &st) != 0) {
                tSystemError("fstat error: {}", strerror(errno));
                co_return;
            }
            const auto fileSize = st.st_size;
#if 0
            void *mapped = mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
            if (mapped == MAP_FAILED) {
                tSystemError("mmap error: {}\n", strerror(errno));
                co_return;
            }

            const int64_t slice_len = 16 * 1024;
            int64_t sent_len = 0;
            while (sent_len < fileSize) {
                int64_t send_len = std::min(fileSize - sent_len, slice_len);
                res = co_await AsyncSend(_sd, (char*)mapped + sent_len, send_len);
                if (res <= 0) {
                    if (res == -EAGAIN || res == -ENOMEM) {
                        continue;
                    }
                    tSystemError("Send error sd={} res={}", _sd, res);
                    break;
                }
                sent_len += res;
            }

            munmap(mapped, fileSize);
#else
            int res = co_await AsyncSendFile(_sd, fd, fileSize);
            tSystemDebug("AsyncSendFile: res:{}", res);

#endif
            _fileName.resize(0);
        }

        if (keepAlivetimeout > 0) {
            timeout = keepAlivetimeout * 1000;  // msecs
        } else {
            break;
        }
    }
}
