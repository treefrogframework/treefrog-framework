#ifndef TCACHEMANAGER_H
#define TCACHEMANAGER_H

#include <TGlobal>


class T_CORE_EXPORT TCacheManager
{
public:
    ~TCacheManager();

    bool open();
    void close();
    bool set(const QByteArray &key, const QByteArray &value, qint64 msecs);
    QByteArray get(const QByteArray &key);
    void remove(const QByteArray &key);
    void clear();

    QString backend() const;
    static TCacheManager &instance();
    static void setCompressionEnabled(bool enable) { compression = enable; }

private:
    static bool compression;
    int _gcDivisor {0};

    T_DISABLE_COPY(TCacheManager)
    T_DISABLE_MOVE(TCacheManager)
    TCacheManager();
};

#endif // TCACHEMANAGER_H
