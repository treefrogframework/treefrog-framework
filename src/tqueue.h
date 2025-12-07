#pragma once
#include "tatomic.h"
#include "tatomicptr.h"
#include "thazardobject.h"
#include "thazardptr.h"


namespace Tf {
T_CORE_EXPORT THazardPtr &hazardPtrForQueue();
}


//
// Non-blocking move queue
//
template <class T>
class TQueue {
private:
    struct Node : public THazardObject {
        T value;
        TAtomicPtr<Node> next;

        explicit Node(const T &v) : value{v} { }
        Node(Node &&) = delete;
        Node &operator=(const Node &) = delete;
        Node &operator=(Node &&) = delete;
    };

    TAtomicPtr<Node> queHead {nullptr};
    TAtomicPtr<Node> queTail {nullptr};
    TAtomic<int> counter {0};

public:
    TQueue();
    void enqueue(T val);
    std::optional<T> dequeue();
    int count() const { return counter.load(); }
    int size() const { return counter.load(); }
    bool empty() const { return counter.load() == 0; }

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
inline void TQueue<T>::enqueue(T val)
{
    auto *newnode = new Node(std::move(val));
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
inline std::optional<T> TQueue<T>::dequeue()
{
    std::optional<T> val;
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
                val = std::move(next->value);
                head->deleteLater();  // gc
                counter--;
                break;
            }
        }
    }
    Tf::hazardPtrForQueue().clear();
    return val;
}
