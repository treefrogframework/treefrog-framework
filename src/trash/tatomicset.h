#ifndef TATOMICSET_H
#define TATOMICSET_H

#include <QAtomicPointer>
#include <QAtomicInt>
#include <QList>
#include <TGlobal>


class T_CORE_EXPORT TAtomicSet
{
public:
    TAtomicSet();
    TAtomicSet(int maxCount);
    TAtomicSet(const QList<void*> items);
    virtual ~TAtomicSet();

    int maxCount() const { return num; }
    void setMaxCount(int count);
    void *pop();
    bool push(void *item);
    int count() const;

    // Peek methods. Call peekPop() and peekPush() by combined use.
    void *peekPop(int i);
    bool peekPush(void *item);

private:
    int num;
    QAtomicPointer<void> *stack;
    QAtomicInt itemCount;

    Q_DISABLE_COPY(TAtomicSet)
};


inline int TAtomicSet::count() const
{
#if QT_VERSION >= 0x050000
    return itemCount.load();
#else
    return (int)itemCount;
#endif
}

#endif // TATOMICSET_H
