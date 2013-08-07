#ifndef TATOMICQUEUE_H
#define TATOMICQUEUE_H

#include <QAtomicPointer>
#include <QList>


template<class T>
class TAtomicQueue
{
public:
    TAtomicQueue() : queue() { }
    void enqueue(const T &t);
    QList<T> dequeue();

private:
    QAtomicPointer<QList<T> > queue;
};


template <class T>
inline void TAtomicQueue<T>::enqueue(const T &t)
{
    QList<T> *newQue = 0;
    for (;;) {
        QList<T> *oldQue = queue.fetchAndStoreOrdered(0);

        if (!newQue) {
            newQue = (oldQue) ? new QList<T>(*oldQue) : new QList<T>();
            newQue->append(t);
        } else {
            if (oldQue) {
                *newQue << *oldQue;
            }
        }

        if (oldQue)
            delete oldQue;

        if (queue.testAndSetOrdered(0, newQue)) {
            break;
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

#endif // TATOMICQUEUE_H
