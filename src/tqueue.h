#ifndef TQUEUE_H
#define TQUEUE_H

#include "thazardobject.h"
#include "thazardptr.h"
#include "tatomicptr.h"

static thread_local THazardPtr hzptrQueue;


template <class T> class TQueue
{
    struct Node : public THazardObject
    {
        T value;
        TAtomicPtr<Node> next;
        Node(const T &v) : value(v) { }
    };

    TAtomicPtr<Node> queHead {nullptr};
    TAtomicPtr<Node> queTail {nullptr};
    std::atomic<int> counter {0};

public:
    TQueue();
    void enqueue(const T &val);
    bool dequeue(T &val);
    int count() const { return counter.load(); }
};


template <class T>
inline TQueue<T>::TQueue()
{
    auto *newnode = new Node(T());
    queHead.store(newnode);
    queTail.store(newnode);
}


template <class T>
inline void TQueue<T>::enqueue(const T &val)
{
   auto *newnode = new Node(val);
   for (;;) {
        Node *tail = queTail.load();
        Node *next = tail->next.load();

        if (Q_UNLIKELY(tail != queTail.load())) {
            continue;
        }

        if (next != nullptr) {
            queTail.compareExchangeStrong(tail, next); // update queTail
            continue;
        }

        if (tail->next.compareExchange(next, newnode)) {
            counter++;
            queTail.compareExchangeStrong(tail, newnode);
            break;
        }
   }
}


template <class T>
inline bool TQueue<T>::dequeue(T &val)
{
    Node *next;
    for (;;) {
        Node *head = queHead.load();
        Node *tail = queTail.load();
        next = hzptrQueue.guard<Node>(&head->next);

        if (Q_UNLIKELY(head != queHead.load())) {
            continue;
        }

        if (head == tail) {
            if (next == nullptr) {
                // no item
                break;
            }
            queTail.compareExchangeStrong(tail, next); // update queTail

        } else {
            if (next == nullptr) {
                continue;
            }

            val = next->value;
            if (queHead.compareExchange(head, next)) {
                head->deleteLater();  // gc
                counter--;
                break;
            }
        }
    }
    hzptrQueue.clear();
    return (bool)next;
}

#endif // TQUEUE_H
