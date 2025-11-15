#pragma once
#include <TAtomic>
#include <TKvsDatabase>
#include <TGlobal>
#include <QBasicTimer>
#include <QStack>
#include <QObject>
#include <QString>
#include <QMutex>

class QSettings;
template <class T> class TStack;


class T_CORE_EXPORT TKvsDatabasePool : public QObject {
    Q_OBJECT
public:
    using TKvsDBConn = std::unique_ptr<TKvsDatabase>;

    ~TKvsDatabasePool();
    TKvsDatabase database(Tf::KvsEngine engine);
    void pool(TKvsDatabase &database);
    TKvsDatabaseSettings getDatabaseSettings(Tf::KvsEngine engine) const;

    static TKvsDatabasePool *instance();

    // TKvsDatabase handle
    class Handle {
    public:
        Handle(TKvsDatabase *ptr) : _ptr(ptr) {}
        Handle(const Handle &) = delete;
        Handle &operator=(const Handle &) = delete;
        Handle(Handle &&other) noexcept
        {
            _ptr  = other._ptr;
            other._ptr = nullptr;
        }

        Handle &operator=(Handle &&other) noexcept
        {
            _ptr  = other._ptr;
            other._ptr = nullptr;
            return *this;
        }

        ~Handle()
        {
            if (_ptr) {
                TKvsDatabasePool::instance()->pool(*_ptr);
            }
        }

        TKvsDatabase *operator->() { return _ptr; }
        TKvsDatabase &operator*()  { return *_ptr; }

    private:
        TKvsDatabase *_ptr {nullptr};
    };

protected:
    void init();
    bool setDatabaseSettings(TKvsDatabase &database, Tf::KvsEngine engine) const;
    void timerEvent(QTimerEvent *event);

    static QString driverName(Tf::KvsEngine engine);

private:
    TKvsDatabasePool();

    mutable QRecursiveMutex _mutex;
    QStack<QString> *cachedDatabase {nullptr};
    TAtomic<uint> *lastCachedTime {nullptr};
    QStack<QString> *availableNames {nullptr};
    int maxConnects {0};
    QBasicTimer timer;

    T_DISABLE_COPY(TKvsDatabasePool)
    T_DISABLE_MOVE(TKvsDatabasePool)
};
