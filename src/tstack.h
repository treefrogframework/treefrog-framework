#ifndef TSTACK_H
#define TSTACK_H

#include "thazardobject.h"
#include "thazardptr.h"
#include "tatomicptr.h"

namespace Tf
{
    static thread_local THazardPtr hzptrStack;
}


template <class T> class TStack
{
    struct Node : public THazardObject
    {
        T value;
        Node *next {nullptr};
        Node(const T &v) : value(v) { }
    };

    TAtomicPtr<Node> stkHead {nullptr};
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
        pnode->next = stkHead.load();
    } while (!stkHead.compareExchange(pnode->next, pnode));
    counter++;
}


template <class T>
inline bool TStack<T>::pop(T &val)
{
    Node *pnode;
    while ((pnode = Tf::hzptrStack.guard<Node>(&stkHead))) {
        if (stkHead.compareExchange(pnode, pnode->next)) {
            break;
        }
    }

    if (pnode) {
        counter--;
        val = pnode->value;
        pnode->next = nullptr;
        pnode->deleteLater();
    }
    Tf::hzptrStack.clear();
    return (bool)pnode;
}


template <class T>
inline bool TStack<T>::top(T &val)
{
    Node *pnode;
    pnode = Tf::hzptrStack.guard<Node>(&stkHead);

    if (pnode) {
        val = pnode->value;
    }
    Tf::hzptrStack.clear();
    return (bool)pnode;
}

#endif // TSTACK_H
