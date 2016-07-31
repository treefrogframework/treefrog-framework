#include <atomic>
#include "thazardobject.h"
#include "thazardpointer.h"

static thread_local THazardPointer hzptr;


template <class T> class stack
{
    struct Node : public THazardObject
    {
        T value;
        Node *next { nullptr };
        Node(const T &v) : value(v) { }
    };

#if 1
    std::atomic<Node*> head { nullptr };
    std::atomic<int> cnt { 0 };
#else
    QAtomicPointer<Node> head { nullptr };
    QAtomicInt cnt { 0 };
#endif

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
#if 1
    auto *pnode = new Node(val);
    do {
  printf("##2\n");
        pnode->next = head.load(std::memory_order_acquire);
    } while (!head.compare_exchange_weak(pnode->next, pnode));
    ++cnt;
#else
    auto *pnode = new Node(val);

    // static int cnt = 0;
    // printf("new obj cnt=%d\n", ++cnt);

    do {
        pnode->next = (Node*)head;
    } while (!head.testAndSetAcquire(pnode->next, pnode));
    cnt.fetchAndAddOrdered(1);
#endif
}


template <class T>
inline bool stack<T>::pop(T &val)
{
    //thread_local THazardPointer hzptr;
    Node *pnode;
#if 1
    while ((pnode = hzptr.guard<Node>(&head))) {
        printf("##\n");
        if (head.compare_exchange_weak(pnode, pnode->next)) {
            break;
        }
    }
#else
    while ((pnode = hzptr.guard<Node>(&head))) {
        if (head.testAndSetOrdered(pnode, pnode->next)) {
            break;
        }
    }
#endif
    if (pnode) {
        val = pnode->value;
        pnode->next = nullptr;
        pnode->deleteLater();
        --cnt;
        //cnt.fetchAndAddOrdered(-1);
    }
    hzptr.clear();
    return (bool)pnode;
}


template <class T>
inline bool stack<T>::peak(T &val)
{
// THazardPointer hzptr;
    Node *pnode;
#if 1
    while ((pnode = hzptr.guard<Node>(&head))) {
        auto next = pnode->next;
  printf("##1\n");
        if (head.compare_exchange_weak(pnode, next)) {
            break;
        }
    }
#else
    while ((pnode = hzptr.guard<Node>(&head))) {
        auto next = pnode->next;
        if (head.testAndSetAcquire(pnode, next)) {
            break;
        }
    }
#endif

    if (pnode) {
        val = pnode->value;
    }
    hzptr.clear();
    return (bool)pnode;
}
