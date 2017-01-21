/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TOption>


TOption operator|(const TOption &o1, const TOption &o2)
{
    TOption options(o1);
    for (QMapIterator<int, QVariant> i(o2); i.hasNext(); ) {
        i.next();
        options.insert(i.key(), i.value());
    }
    return options;
}
