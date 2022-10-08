/* Copyright (c) 2015-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tmemcacheddriver.h"
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

/*!
  Constructs a TMemcached object.
*/
TMemcached::TMemcached() :
    database(Tf::currentDatabaseContext()->getKvsDatabase(Tf::KvsEngine::Memcached))
{
}

TMemcached::TMemcached(Tf::KvsEngine engine) :
    database(Tf::currentDatabaseContext()->getKvsDatabase(engine))
{
}

/*!
  Copy constructor.
*/
TMemcached::TMemcached(const TMemcached &other) :
    database(other.database)
{
}

/*!
  Returns the Memcached driver associated with the TMemcached object.
*/
TMemcachedDriver *TMemcached::driver()
{
#ifdef TF_NO_DEBUG
    return (TMemcachedDriver *)database.driver();
#else
    if (!database.driver()) {
        return nullptr;
    }

    TMemcachedDriver *driver = dynamic_cast<TMemcachedDriver *>(database.driver());
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
    return (const TMemcachedDriver *)database.driver();
#else
    if (!database.driver()) {
        return nullptr;
    }

    const TMemcachedDriver *driver = dynamic_cast<const TMemcachedDriver *>(database.driver());
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
