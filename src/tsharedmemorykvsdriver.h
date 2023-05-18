#pragma once
#include <QString>
#include <QtGlobal>
#include <TGlobal>
#include <TKvsDriver>

class TSharedMemoryAllocator;


class T_CORE_EXPORT TSharedMemoryKvsDriver : public TKvsDriver {
public:
    TSharedMemoryKvsDriver();
    ~TSharedMemoryKvsDriver();

    QString key() const override { return "MEMORY"; }
    bool open(const QString &db, const QString &user = QString(), const QString &password = QString(), const QString &host = QString(), uint16_t port = 0, const QString &options = QString()) override;
    void close() override;
    bool isOpen() const override { return _size > 0; }
    QString name() const { return _name; }
    size_t size() const { return _size; }

    void *malloc(uint size);
    void *calloc(uint num, uint nsize);
    void *realloc(void *ptr, uint size);
    void free(void *ptr);
    uint allocSize(const void *ptr) const;
    size_t mapSize() const;
    void *origin() const;

    bool lockForRead();
    bool lockForWrite();
    bool unlock();

private:
    static void initialize(const QString &db, const QString &options);
    void cleanup();

    QString _name;
    size_t _size {0};
    TSharedMemoryAllocator *_allocator {nullptr};

    friend class TSharedMemoryKvs;
    T_DISABLE_COPY(TSharedMemoryKvsDriver)
    T_DISABLE_MOVE(TSharedMemoryKvsDriver)
};
