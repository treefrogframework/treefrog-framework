#pragma once
#include "thazardobject.h"
#include "thazardptr.h"


template <class T>
class TSinglyList {
public:
    TSinglyList() { }

    const T value(const QString &key, const T &defaultValue = T());
    void insert(const QString &key, const T &value);
    int remove(const QString &key);
    T take(const QString &key);

private:
    struct Node : public THazardObject {
        QString key;
        T value;
        TAtomicPtr<Node> next {nullptr};

        Node(const QString &k, const T &v) :
            key(k), value(v) { }
    };

    TAtomicPtr<Node> head {nullptr};
    static thread_local THazardPtr hzptr;

    T_DISABLE_COPY(TSinglyList)
    T_DISABLE_MOVE(TSinglyList)
};


template <class T>
thread_local THazardPtr TSinglyList<T>::hzptr;


template <class T>
inline const T TSinglyList<T>::value(const QString &key, const T &defaultValue)
{
    boo mark;
    Node *prev = nullptr;
    Node *ptr = hzptr.guard<Node>(&head, &mark);
    if (ptr && mark) {
        while (mark) {
            ptr = hzptr.guard<Node>(ptr->next, &mark);
        }
        ? ? ?
    }

    while (ptr) {
        if (mark) {
            // gc
            if (prev) {
                prev->next = ptr->next;
            }
        } else if (ptr->key == key) {
            break;
        } else {
            prev = ptr;
        }
        ptr = hzptr.guard<Node>(&ptr->next, &mark);
    }
    return (ptr) ? ptr->value : defaultValue;
}


template <class T>
inline void TSinglyList<T>::insert(const QString &key, const T &value)
{
    auto *pnode = new Node(key, value);
    Node *oldp;
    do {
        oldp = head.load();
        pnode->next.store(oldp);
    } while (!head.compareExchange(oldp, pnode));
}


template <class T>
inline int TSinglyList<T>::remove(const QString &key)
{
    return 0;
}


template <class T>
inline T TSinglyList<T>::take(const QString &key)
{
    return T();
}

