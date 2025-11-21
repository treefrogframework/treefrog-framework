#pragma once
#include <TAtomic>
#include <TKvsDatabase>
#include <TGlobal>
#include <QBasicTimer>
#include <QStack>
#include <QObject>
#include <QString>
#include <QMutex>
#include <vector>
#include <deque>


class T_CORE_EXPORT TKvsDatabasePool : public QObject {
    Q_OBJECT
public:
    using KvsDbPtr = std::unique_ptr<TKvsDatabase>;

    template <class T>
    class MoveStack : public std::deque<T> {
    public:
        MoveStack() = default;
        MoveStack(const MoveStack &) = delete;
        MoveStack &operator=(const MoveStack &) = delete;
        MoveStack(MoveStack &&) noexcept = default;
        MoveStack &operator=(MoveStack &&) noexcept = default;
        void push(T value) { std::deque<T>::push_back(std::move(value)); }
        T pop()
        {
            T v = std::move(std::deque<T>::back());
            std::deque<T>::pop_back();
            return v;
        }
        T &top() { return std::deque<T>::back(); }
        const T &top() const { return std::deque<T>::back(); }
    };

    ~TKvsDatabasePool();
    TKvsDatabase::Handle database(Tf::KvsEngine engine);
    TKvsDatabaseSettings getDatabaseSettings(Tf::KvsEngine engine) const;

    static TKvsDatabasePool *instance();

protected:
    void init();
    bool setDatabaseSettings(TKvsDatabase &database, Tf::KvsEngine engine) const;
    void timerEvent(QTimerEvent *event);

    static QString driverName(Tf::KvsEngine engine);

private:
    TKvsDatabasePool();
    void pool(KvsDbPtr dbptr);

    mutable QRecursiveMutex _mutex;
    TAtomic<uint> *lastCachedTime {nullptr};
    int maxConnects {0};
    QBasicTimer timer;
    std::vector<MoveStack<KvsDbPtr>> availableDatabases;
    std::vector<MoveStack<KvsDbPtr>> cachedDatabases;

    T_DISABLE_COPY(TKvsDatabasePool)
    T_DISABLE_MOVE(TKvsDatabasePool)
    friend class TKvsDatabase;
};
