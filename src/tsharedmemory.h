#pragma once
#include <TGlobal>


class T_CORE_EXPORT TSharedMemory
#ifndef Q_OS_LINUX
 : public QQSharedMemory
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
    QString name() const { return _name; }
    size_t size() const { return _size; }

    bool lockForRead();
    bool lockForWrite();
    bool unlock();

private:
#ifdef Q_OS_LINUX
    QString _name;
    size_t _size {0};
    void *_ptr {nullptr};
    int _fd {0};
#endif

    T_DISABLE_COPY(TSharedMemory)
    T_DISABLE_MOVE(TSharedMemory)
};
