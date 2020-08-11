#pragma once
#include "tatomic.h"
#include "tatomicptr.h"
#include "thazardobject.h"
#include "thazardptr.h"
#include <TGlobal>


namespace Tf {
T_CORE_EXPORT THazardPtr &hazardPtrForStack();
}


template <class T>
class TStack {
private:
    struct Node : public THazardObject {
        T value;
        Node *next {nullptr};
        Node(const T &v) :
            value(v) { }
    };

    TAtomicPtr<Node> stkHead {nullptr};
    TAtomic<int> counter {0};

public:
    TStack() { }
    void push(const T &val);
    bool pop(T &val);
    bool top(T &val);
    int count() const { return counter.load(); }

    T_DISABLE_COPY(TStack)
    T_DISABLE_MOVE(TStack)
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
    while ((pnode = Tf::hazardPtrForStack().guard<Node>(&stkHead))) {
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
    Tf::hazardPtrForStack().clear();
    return (bool)pnode;
}


template <class T>
inline bool TStack<T>::top(T &val)
{
    Node *pnode;
    pnode = Tf::hazardPtrForStack().guard<Node>(&stkHead);

    if (pnode) {
        val = pnode->value;
    }
    Tf::hazardPtrForStack().clear();
    return (bool)pnode;
}

