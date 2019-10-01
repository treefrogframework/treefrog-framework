/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TOption>


TOption operator|(const TOption &o1, const TOption &o2)
{
    TOption options(o1);
    for (auto it = o2.begin(); it != o2.end(); ++it) {
        options.insert(it.key(), it.value());
    }
    return options;
}
