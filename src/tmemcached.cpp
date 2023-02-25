/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tmemcacheddriver.h"
#include "tsystemglobal.h"
#include <TActionContext>
#include <TMemcached>

/*!
  \class TMemcached
  \brief The TMemcached class provides a means of operating a Memcached
  system.

  Edit conf/memcached.ini and conf/application.ini to use this class.

  memcached.ini:
  \code
    HostName=xxx.xxx.xxx.xxx
    UserName=
    Password=
  \endcode

  application.ini:
  \code
    MemcachedSettingsFile=memcached.ini
  \endcode
  <a href="https://github.com/memcached/memcached/wiki">See also memcached documentation.</a>
*/

namespace {

inline bool containsWhiteSpace(const QByteArray &string)
{
    for (char c : string) {
        if (c <= ' ' || c >= 0x7F) {
            return true;
        }
    }
    return false;
}

}

/*!
  Constructs a TMemcached object.
*/
TMemcached::TMemcached() :
    _database(Tf::currentDatabaseContext()->getKvsDatabase(Tf::KvsEngine::Memcached))
{
}

TMemcached::TMemcached(Tf::KvsEngine engine) :
    _database(Tf::currentDatabaseContext()->getKvsDatabase(engine))
{
}


QByteArray TMemcached::get(const QByteArray &key, uint *flags)
{
    QByteArray res = requestLine("get", key, QByteArray(), false);
    //tSystemDebug("TMemcached::get: %s", res.data());

    int bytes = 0;
    int pos = 0;
    int to = res.indexOf(Tf::CRLF, pos);
    if (to > 0) {
        QByteArray line = res.mid(pos, to - pos);  // First line
        pos = to + 2;

        auto strs = line.split(' ');
        if (flags) {
            *flags = strs.value(2).toUInt();  // flags
        }
        bytes = strs.value(3).toInt();  // bytes
        res = res.remove(0, pos);
    }
    res.resize(bytes);
    return res;
}


qint64 TMemcached::getNumber(const QByteArray &key, bool *ok, uint *flags)
{
    QByteArray res = get(key, flags);

    if (ok) {
        *ok = false;
    }

    if (res.isEmpty()) {
        return 0;
    }

    return res.toLongLong(ok);
}


bool TMemcached::set(const QByteArray &key, const QByteArray &value, int seconds, uint flags)
{
    QByteArray res = request("set", key, value, flags, seconds, false);
    return res.startsWith("STORED");
}


bool TMemcached::set(const QByteArray &key, qint64 value, int seconds, uint flags)
{
    return set(key, QByteArray::number(value), seconds, flags);
}


bool TMemcached::add(const QByteArray &key, const QByteArray &value, int seconds, uint flags)
{
    QByteArray res = request("add", key, value, flags, seconds, false);
    return res.startsWith("STORED");
}


bool TMemcached::add(const QByteArray &key, qint64 value, int seconds, uint flags)
{
    return add(key, QByteArray::number(value), seconds, flags);

}


bool TMemcached::replace(const QByteArray &key, const QByteArray &value, int seconds, uint flags)
{
    QByteArray res = request("replace", key, value, flags, seconds, false);
    return res.startsWith("STORED");
}


bool TMemcached::replace(const QByteArray &key, qint64 value, int seconds, uint flags)
{
    return replace(key, QByteArray::number(value), seconds, flags);
}


bool TMemcached::append(const QByteArray &key, const QByteArray &value, int seconds, uint flags)
{
    QByteArray res = request("append", key, value, flags, seconds, false);
    return res.startsWith("STORED");
}


bool TMemcached::prepend(const QByteArray &key, const QByteArray &value, int seconds, uint flags)
{
    QByteArray res = request("prepend", key, value, flags, seconds, false);
    return res.startsWith("STORED");
}


bool TMemcached::remove(const QByteArray &key)
{
    QByteArray res = requestLine("delete", key, QByteArray(), false);
    return res.startsWith("DELETED");
}


quint64 TMemcached::incr(const QByteArray &key, quint64 value, bool *ok)
{
    QByteArray res = requestLine("incr", key, QByteArray::number(value), false);
    if (res.startsWith("NOT_FOUND")) {
        if (ok) {
            *ok = false;
        }
        return 0;
    }
    return res.toLongLong(ok);
}


quint64 TMemcached::decr(const QByteArray &key, quint64 value, bool *ok)
{
    QByteArray res = requestLine("decr", key, QByteArray::number(value), false);
    if (res.startsWith("NOT_FOUND")) {
        if (ok) {
            *ok = false;
        }
        return 0;
    }
    return res.toLongLong(ok);
}


bool TMemcached::flushAll()
{
    QByteArray res = requestLine("flush_all", QByteArray(), QByteArray(), false);
    return res.startsWith("OK");
}


QByteArray TMemcached::version()
{
    QByteArray res = requestLine("version", QByteArray(), QByteArray(), false);
    int idx = res.indexOf(' ');
    return res.mid(idx + 1).trimmed();
}


QByteArray TMemcached::request(const QByteArray &command, const QByteArray &key, const QByteArray &value, uint flags, int exptime, bool noreply)
{
    QByteArray message;
    message.reserve(key.length() + value.length() + 32);

    if (key.isEmpty() || containsWhiteSpace(key)) {
        tError("Value error, key: %s", key.data());
        return QByteArray();
    }

    if (!isOpen()) {
        tSystemError("Not open memcached  [%s:%d]", __FILE__, __LINE__);
        return QByteArray();
    }

    message += command;
    message += " ";
    message += key;
    message += " ";
    message += QByteArray::number(flags);
    if (exptime >= 0) {
        message += " ";
        message += QByteArray::number(exptime);
    }
    message += " ";
    message += QByteArray::number(value.length());
    if (noreply) {
        message += " ";
        message += "noreply";
    }
    message += Tf::CRLF;
    message += value;
    message += Tf::CRLF;
    //tSystemDebug("memcached message: %s", message.data());

    int timeout = (noreply) ? 0 : 5000;
    return driver()->request(message, timeout);
}

// Requests command in single line. For incr or decr.
QByteArray TMemcached::requestLine(const QByteArray &command, const QByteArray &key, const QByteArray &value, bool noreply)
{
    QByteArray message;
    message.reserve(key.length() + value.length() + 24);

    if (containsWhiteSpace(key) || containsWhiteSpace(value)) {
        tError("Key or value error, key:%s value:%s", key.data(), value.data());
        return QByteArray();
    }

    if (!isOpen()) {
        tSystemError("Not open memcached  [%s:%d]", __FILE__, __LINE__);
        return QByteArray();
    }

    message += command;
    if (!key.isEmpty()) {
        message += " ";
        message += key;
    }
    if (!value.isEmpty()) {
        message += " ";
        message += value;
    }
    if (noreply) {
        message += " ";
        message += "noreply";
    }
    message += Tf::CRLF;
    //tSystemDebug("memcached message: %s", message.data());

    int timeout = (noreply) ? 0 : 5000;
    return driver()->request(message, timeout);
}


/*!
  Returns the Memcached driver associated with the TMemcached object.
*/
TMemcachedDriver *TMemcached::driver()
{
#ifdef TF_NO_DEBUG
    return (TMemcachedDriver *)_database.driver();
#else
    if (!_database.driver()) {
        return nullptr;
    }

    TMemcachedDriver *driver = dynamic_cast<TMemcachedDriver *>(_database.driver());
    if (!driver) {
        throw RuntimeException("cast error", __FILE__, __LINE__);
    }
    return driver;
#endif
}

/*!
  Returns the Memcached driver associated with the TMemcached object.
*/
const TMemcachedDriver *TMemcached::driver() const
{
#ifdef TF_NO_DEBUG
    return (const TMemcachedDriver *)_database.driver();
#else
    if (!_database.driver()) {
        return nullptr;
    }

    const TMemcachedDriver *driver = dynamic_cast<const TMemcachedDriver *>(_database.driver());
    if (!driver) {
        throw RuntimeException("cast error", __FILE__, __LINE__);
    }
    return driver;
#endif
}

/*!
  Returns true if the Memcached connection is open; otherwise
  returns false.
 */
bool TMemcached::isOpen() const
{
    return (driver()) ? driver()->isOpen() : false;
}
