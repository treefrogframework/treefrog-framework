/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TDirectView>
#include <TActionController>
#include <TSession>


const TSession &TDirectView::session() const
{
    return controller()->session();
}


TSession &TDirectView::session()
{
    return actionController->session();
}


bool TDirectView::addCookie(const TCookie &cookie)
{
    return actionController->addCookie(cookie);
}


bool TDirectView::addCookie(const QByteArray &name, const QByteArray &value, const QDateTime &expire, const QString &path, const QString &domain, bool secure, bool httpOnly)
{
    return actionController->addCookie(name, value, expire, path, domain, secure, httpOnly);
}
