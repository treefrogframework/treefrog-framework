#ifndef TATOMICQUEUE_H
#define TATOMICQUEUE_H

#include <atomic>
#include <QList>
#include <QMutex>
#include <QWaitCondition>


template<class T>
class TAtomicQueue
{
public:
    TAtomicQueue() : queue(0), mutex(), enqued() { }
    void enqueue(const T &t);
    QList<T> dequeue();
    bool wait(int timeout);

private:
    std::atomic<void *> queue;
    QMutex mutex;
    QWaitCondition enqued;
};


template <class T>
inline void TAtomicQueue<T>::enqueue(const T &t)
{
    QList<T> *newQue = new QList<T>();
    newQue->append(t);

    for (;;) {
        void *ptr = nullptr;
        if (queue.compare_exchange_strong(ptr, newQue)) {
            enqued.wakeOne();
            break;
        }

        QList<T> *oldQue = (QList<T> *)queue.exchange(nullptr);
        if (oldQue) {
            *oldQue << *newQue;
            delete newQue;
            newQue = oldQue;
        }
    }
}


template <class T>
inline QList<T> TAtomicQueue<T>::dequeue()
{
    QList<T> ret;
    QList<T> *ptr = (QList<T> *)queue.exchange(nullptr);

    if (ptr) {
        ret = *ptr;
        delete ptr;
    }
    return ret;
}


template <class T>
inline bool TAtomicQueue<T>::wait(int timeout)
{
    mutex.lock();
    bool ret = enqued.wait(&mutex, timeout);
    mutex.unlock();
    return ret;
}

#endif // TATOMICQUEUE_H
