#pragma once
#include "tatomic.h"
#include "tatomicptr.h"
#include "thazardobject.h"
#include "thazardptr.h"
#include "tdeclexport.h"
#include <TSystemGlobal>


namespace Tf {
T_CORE_EXPORT THazardPtr &hazardPtrForStack();
}


//
// Non-blocking move stack
//
template <class T>
class TStack {
private:
    struct Node : public THazardObject {
        T value;
        Node *next {nullptr};

        Node(T &&v) : value(std::move(v)) { }
        Node(const Node &) = delete;
        Node &operator=(const Node &) = delete;
        Node(Node &&) = delete;
        Node &operator=(Node &&) = delete;
    };

    TAtomicPtr<Node> stkHead {nullptr};
    TAtomic<int> counter {0};

public:
    TStack() = default;
    TStack(const TStack &) = delete;
    TStack &operator=(const TStack &) = delete;
    TStack(TStack &&) noexcept;
    TStack &operator=(TStack &&) = delete;

    void push(T val);
    T pop(bool *ok = nullptr);
    int count() const { return counter.load(); }
    int size() const { return counter.load(); }
    bool empty() const { return counter.load() == 0; }
};


template <class T>
inline TStack<T>::TStack(TStack &&other) noexcept
{
    stkHead = other.stkHead;
    counter.store(other.counter.load());
}


template <class T>
inline void TStack<T>::push(T val)
{
    auto *pnode = new Node(std::move(val));
    do {
        pnode->next = stkHead.load();
    } while (!stkHead.compareExchange(pnode->next, pnode));
    counter++;
}


template <class T>
inline T TStack<T>::pop(bool *ok)
{
    T val;
    Node *pnode;
    while ((pnode = Tf::hazardPtrForStack().guard<Node>(&stkHead))) {
        if (stkHead.compareExchange(pnode, pnode->next)) {
            break;
        }
    }

    if (pnode) {
        counter--;
        val = std::move(pnode->value);
        pnode->next = nullptr;
        pnode->deleteLater();

        if (ok) {
            *ok = true;
        }
    } else {
        if (ok) {
            *ok = false;
        }
    }
    Tf::hazardPtrForStack().clear();
    return val;
}


/*
template <class T>
class TMoveStack : public std::deque<T> {
public:
    TMoveStack() = default;
    TMoveStack(const TMoveStack &) = delete;
    TMoveStack &operator=(const TMoveStack &) = delete;
    TMoveStack(TMoveStack &&) noexcept = default;
    TMoveStack &operator=(TMoveStack &&) noexcept = default;
    void push(T value) { std::deque<T>::push_back(std::move(value)); }
    T pop()
    {
        T v = std::move(std::deque<T>::back());
        std::deque<T>::pop_back();
        return v;
    }
    T &top() { return std::deque<T>::back(); }
    const T &top() const { return std::deque<T>::back(); }
};
*/
