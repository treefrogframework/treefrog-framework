#pragma once
#include <TGlobal>

class TCacheStore;


class T_CORE_EXPORT TCache {
public:
    TCache();
    ~TCache();

    bool set(const QByteArray &key, const QByteArray &value, int seconds);
    QByteArray get(const QByteArray &key);
    void remove(const QByteArray &key);
    void clear();

    static bool compressionEnabled();

private:
    void initialize();
    void cleanup();

    TCacheStore *_cache {nullptr};
    int _gcDivisor {0};

    friend class TWebApplication;
    T_DISABLE_COPY(TCache)
    T_DISABLE_MOVE(TCache)
};
