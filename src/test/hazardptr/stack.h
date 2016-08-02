#include <QAtomicInt>
#include "thazardobject.h"
#include "thazardpointer.h"
#include "tatomicptr.h"

static thread_local THazardPointer hzptr;


template <class T> class stack
{
    struct Node : public THazardObject
    {
        T value;
        Node *next { nullptr };
        Node(const T &v) : value(v) { }
    };

    TAtomicPtr<Node> head { nullptr };
    QAtomicInt cnt { 0 };

public:
    stack() { }
    void push(const T &val);
    bool pop(T &val);
    bool peak(T &val);
    int count() { return cnt.load(); }
};


template <class T>
inline void stack<T>::push(const T &val)
{
    auto *pnode = new Node(val);
    do {
        pnode->next = head.load();
    } while (!head.compareExchange(pnode->next, pnode));
    ++cnt;
}


template <class T>
inline bool stack<T>::pop(T &val)
{
    //thread_local THazardPointer hzptr;
    Node *pnode;
    while ((pnode = hzptr.guard<Node>(&head))) {
        if (head.compareExchange(pnode, pnode->next)) {
            break;
        }
    }

    if (pnode) {
        val = pnode->value;
        pnode->next = nullptr;
        pnode->deleteLater();
        --cnt;
    }
    hzptr.clear();
    return (bool)pnode;
}


template <class T>
inline bool stack<T>::peak(T &val)
{
// THazardPointer hzptr;
    Node *pnode;
    pnode = hzptr.guard<Node>(&head);

    if (pnode) {
        val = pnode->value;
    }
    hzptr.clear();
    return (bool)pnode;
}
