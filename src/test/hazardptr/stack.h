#include <atomic>
#include "thazardobject.h"
#include "thazardpointer.h"


template <class T> class stack
{
    struct Node : public THazardObject
    {
        T value;
        Node *next { nullptr };
        Node(const T &v) : value(v) { }
    };

    std::atomic<Node*> head { nullptr };
    std::atomic<int> cnt { 0 };

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
    } while (!head.compare_exchange_weak(pnode->next, pnode));
    ++cnt;
}


template <class T>
inline bool stack<T>::pop(T &val)
{
    THazardPointer hzptr;
    Node *pnode;
    while ((pnode = head.load())) {
        if (!hzptr.guard<Node>(pnode, &head)) {
            continue;
        }
        auto next = pnode->next;
        if (head.compare_exchange_weak(pnode, next)) {
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
    THazardPointer hzptr;
    Node *pnode;
    while ((pnode = head.load())) {
        if (!hzptr.guard(pnode, &head)) {
            continue;
        }
        auto next = pnode->next;
        if (head.compare_exchange_weak(pnode, next)) {
            break;
        }
    }

    if (pnode) {
        val = pnode->value;
    }
    hzptr.clear();
    return (bool)pnode;
}
