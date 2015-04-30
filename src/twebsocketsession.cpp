/* Copyright (c) 2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TWebSocketSession>
#include <TSession>


TWebSocketSession &TWebSocketSession::unite(const TSession &session)
{
    QVariantMap::unite(*static_cast<const QVariantMap*>(&session));
    return *this;
}
