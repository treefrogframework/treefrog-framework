#ifndef TSTACK_H
#define TSTACK_H

#include "thazardobject.h"
#include "thazardptr.h"
#include "tatomicptr.h"

static thread_local THazardPtr hzptr;


template <class T> class TStack
{
    struct Node : public THazardObject
    {
        T value;
        Node *next {nullptr};
        Node(const T &v) : value(v) { }
    };

    TAtomicPtr<Node> head {nullptr};
    std::atomic<int> counter {0};

public:
    TStack() {}
    void push(const T &val);
    bool pop(T &val);
    bool top(T &val);
    int count() const { return counter.load(); }
};


template <class T>
inline void TStack<T>::push(const T &val)
{
    auto *pnode = new Node(val);
    do {
        pnode->next = head.load();
    } while (!head.compareExchange(pnode->next, pnode));
    counter++;
}


template <class T>
inline bool TStack<T>::pop(T &val)
{
    Node *pnode;
    while ((pnode = hzptr.guard<Node>(&head))) {
        if (head.compareExchange(pnode, pnode->next)) {
            break;
        }
    }

    if (pnode) {
        counter--;
        val = pnode->value;
        pnode->next = nullptr;
        pnode->deleteLater();
    }
    hzptr.clear();
    return (bool)pnode;
}


template <class T>
inline bool TStack<T>::top(T &val)
{
    Node *pnode;
    pnode = hzptr.guard<Node>(&head);

    if (pnode) {
        val = pnode->value;
    }
    hzptr.clear();
    return (bool)pnode;
}

#endif // TSTACK_H
