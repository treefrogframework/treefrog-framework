#pragma once
#include <TGlobal>


class T_CORE_EXPORT TSharedMemory {
public:
    TSharedMemory(const QString &name);
    ~TSharedMemory();
    bool create(size_t size);
    void unlink();
    bool attach();
    bool detach();

    void *data() { return _data; }
    const void *data() const { return _data; }
    size_t size() const { return _size; }

    bool lock();
    bool unlock();

private:
    QString _name;
    size_t _size {0};
    void *_data {nullptr};
    int _fd {0};
};
