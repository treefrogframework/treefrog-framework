#pragma once
#include <TGlobal>
#ifdef Q_OS_WIN
#include <QSharedMemory>
#endif
#ifdef Q_OS_LINUX
#include <pthread.h>
#endif


class T_CORE_EXPORT TSharedMemory
#ifdef Q_OS_WIN
 : public QSharedMemory
#endif
{
public:
    TSharedMemory(const QString &name);
    ~TSharedMemory();
    bool create(size_t size);
    void unlink();
    bool attach();
    bool detach();

    void *data();
    const void *data() const;
    QString name() const;
    size_t size() const;

    bool lockForRead();
    bool lockForWrite();
    bool unlock();

private:
    struct header_t {
#ifdef Q_OS_LINUX
        pthread_rwlock_t rwlock;
#endif
        uint lockcounter {0};
    };

    bool initRwlock(header_t *header) const;
    void releaseRwlock(header_t *header) const;


#ifndef Q_OS_WIN
    QString _name;
    size_t _size {0};
    void *_ptr {nullptr};
    int _fd {0};
#endif

    T_DISABLE_COPY(TSharedMemory)
    T_DISABLE_MOVE(TSharedMemory)
};
