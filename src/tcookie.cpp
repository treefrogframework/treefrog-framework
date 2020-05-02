/* Copyright (c) 2020, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TCookie>

static const QByteArrayList SameSiteValidations = {"strict", "lax", "none"};


TCookie::TCookie(const QByteArray &name, const QByteArray &value) :
    QNetworkCookie(name, value)
{
}


TCookie::TCookie(const TCookie &other) :
    QNetworkCookie(other),
    _maxAge(other._maxAge),
    _sameSite(other._sameSite)
{
}


TCookie::TCookie(const QNetworkCookie &other) :
    QNetworkCookie(other)
{
}


TCookie &TCookie::operator=(const TCookie &other)
{
    QNetworkCookie::operator=(other);
    _maxAge = other._maxAge;
    _sameSite = other._sameSite;
    return *this;
}


bool TCookie::setSameSite(const QByteArray &sameSite)
{
    if (sameSite.isEmpty() || SameSiteValidations.contains(sameSite.toLower())) {
        _sameSite = sameSite;
        return true;
    }
    return false;
}


void TCookie::swap(TCookie &other)
{
    QNetworkCookie::swap(other);
    std::swap(_maxAge, other._maxAge);
    std::swap(_sameSite, other._sameSite);
}


QByteArray TCookie::toRawForm(QNetworkCookie::RawForm form) const
{
    QByteArray raw = QNetworkCookie::toRawForm(form);

    if (_maxAge > 0) {
        raw += "; Max-Age=";
        raw += QByteArray::number(_maxAge);
    }

    if (!_sameSite.isEmpty()) {
        raw += "; SameSite=";
        raw += _sameSite;
    }
    return raw;
}


bool TCookie::operator!=(const TCookie &other) const
{
    return !operator==(other);
}


bool TCookie::operator==(const TCookie &other) const
{
    return QNetworkCookie::operator==(other) && _maxAge == other._maxAge && _sameSite == other._sameSite;
}


QList<TCookie> TCookie::parseCookies(const QByteArray &cookieString)
{
    QList<TCookie> ret;
    auto cookieList = QNetworkCookie::parseCookies(cookieString);
    for (auto &cookie : cookieList) {
        ret << TCookie(cookie);
    }
    return ret;
}
