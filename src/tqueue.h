#pragma once
#include "tatomic.h"
#include "tatomicptr.h"
#include "thazardobject.h"
#include "thazardptr.h"
#include <TGlobal>


namespace Tf {
T_CORE_EXPORT THazardPtr &hazardPtrForQueue();
}


template <class T>
class TQueue {
private:
    struct Node : public THazardObject {
        T value;
        TAtomicPtr<Node> next;
        Node(const T &v) :
            value(v) { }
    };

    TAtomicPtr<Node> queHead {nullptr};
    TAtomicPtr<Node> queTail {nullptr};
    TAtomic<int> counter {0};

public:
    TQueue();
    void enqueue(const T &val);
    bool dequeue(T &val);
    bool head(T &val);
    int count() const { return counter.load(); }

    T_DISABLE_COPY(TQueue)
    T_DISABLE_MOVE(TQueue)
};


template <class T>
inline TQueue<T>::TQueue()
{
    auto dummy = new Node(T());  // dummy node
    queHead.store(dummy);
    queTail.store(dummy);
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

        if (next) {
            queTail.compareExchange(tail, next);  // update queTail
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
        next = Tf::hazardPtrForQueue().guard<Node>(&head->next);

        if (Q_UNLIKELY(head != queHead.load())) {
            continue;
        }

        if (head == tail) {
            if (next) {
                queTail.compareExchange(tail, next);  // update queTail
            } else {
                // no item
                break;
            }
        } else {
            if (Q_UNLIKELY(!next)) {
                continue;
            }

            if (queHead.compareExchange(head, next)) {
                val = next->value;
                head->deleteLater();  // gc
                counter--;
                break;
            }
        }
    }
    Tf::hazardPtrForQueue().clear();
    return (bool)next;
}


template <class T>
inline bool TQueue<T>::head(T &val)
{
    Node *next;
    for (;;) {
        Node *headp = queHead.load();
        next = Tf::hazardPtrForQueue().guard<Node>(&headp->next);

        if (Q_LIKELY(headp == queHead.load())) {
            if (next) {
                val = next->value;
            }
            break;
        }
    }
    Tf::hazardPtrForQueue().clear();
    return (bool)next;
}

