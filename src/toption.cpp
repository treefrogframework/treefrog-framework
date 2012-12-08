/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TOption>

#if 0
TOption::TOption(const TOption &other)
    : QHash<int, QVariant>(*static_cast<const QHash<int, QVariant> *>(&other))
{ }


TOption::TOption(const QHash<int, QVariant> hash)
    : QHash<int, QVariant>(hash)
{ }


TOption &TOption::operator=(const TOption &other)
{
    QHash<int, QVariant>::operator=(*static_cast<const QHash<int, QVariant> *>(&other));
    return *this;
}


TOption TOption::operator|(const TOption &other) const
{
    TOption options(*this);
    for (QHashIterator<int, QVariant> i(other); i.hasNext(); ) {
        i.next();
        options.insert(i.key(), i.value());
    }
    return options;
}
#endif


TOption operator|(const TOption &o1, const TOption &o2)
{
    TOption options(o1);
    for (QHashIterator<int, QVariant> i(o2); i.hasNext(); ) {
        i.next();
        options.insert(i.key(), i.value());
    }
    return options;
}
