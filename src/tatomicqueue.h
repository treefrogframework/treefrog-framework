#ifndef TATOMICQUEUE_H
#define TATOMICQUEUE_H

#include <QAtomicPointer>
#include <QList>
#include <QMutex>
#include <QWaitCondition>


template<class T>
class TAtomicQueue
{
public:
    TAtomicQueue() : queue(), mutex(), enqued() { }
    void enqueue(const T &t);
    QList<T> dequeue();
    bool wait(int timeout);

private:
    QAtomicPointer<QList<T> > queue;
    QMutex mutex;
    QWaitCondition enqued;
};


template <class T>
inline void TAtomicQueue<T>::enqueue(const T &t)
{
    QList<T> *newQue = new QList<T>();
    newQue->append(t);

    for (;;) {
        if (queue.testAndSetOrdered(NULL, newQue)) {
            enqued.wakeOne();
            break;
        }

        QList<T> *oldQue = queue.fetchAndStoreOrdered(NULL);
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
    QList<T> *ptr = queue.fetchAndStoreOrdered(0);

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
