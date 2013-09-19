/* Copyright (c) 2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QAtomicPointer>
#include <QThread>
#include "tatomicset.h"
#include "tsystemglobal.h"


TAtomicSet::TAtomicSet()
    : num(0), stack(0), itemCount(0)
{ }


TAtomicSet::TAtomicSet(int maxCount)
    : num(maxCount), stack(new QAtomicPointer<void>[maxCount]), itemCount(0)
{ }


TAtomicSet::TAtomicSet(const QList<void*> items)
    :  num(items.count()), stack(new QAtomicPointer<void>[items.count()]), itemCount(0)
{
    for (QListIterator<void*> it(items); it.hasNext(); ) {
        push(it.next());
    }
}


void TAtomicSet::setMaxCount(int count)
{
    Q_ASSERT(!stack);
    stack = new QAtomicPointer<void>[count];
    num = count;
    itemCount = 0;
}


TAtomicSet::~TAtomicSet()
{
    if (stack)
        delete[] stack;
}


void *TAtomicSet::pop()
{
    void *ret = 0;

    while (count() > 0) {
        for (int i = 0; i < num; ++i) {
            ret = stack[i].fetchAndStoreOrdered(0);
            if (ret) {
                itemCount.fetchAndAddOrdered(-1);
                return ret;
            }
        }
        QThread::yieldCurrentThread();
    }
    return ret;
}


bool TAtomicSet::push(void *item)
{
    Q_CHECK_PTR(item);
    bool ret = false;

    while (count() < num) {
        for (int i = 0; i < num; ++i) {
            ret = stack[i].testAndSetOrdered(0, item);
            if (ret) {
                itemCount.fetchAndAddOrdered(1);
                return ret;
            }
        }
        QThread::yieldCurrentThread();
    }
    return ret;
}


// pop item without counter decrement
void *TAtomicSet::peekPop(int i)
{
    return (stack) ? stack[i].fetchAndStoreOrdered(0) : 0;
}

// push item without counter increment
bool TAtomicSet::peekPush(void *item)
{
    Q_CHECK_PTR(item);
    bool ret = false;

    if (!stack)
        return ret;

    for (int i = 0; i < num; ++i) {
        ret = stack[i].testAndSetOrdered(0, item);
        if (ret) {
            return ret;
        }
    }
    return ret;
}
