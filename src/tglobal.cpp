/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "lz4.h"
#include "tdatabasecontextthread.h"
#include <QBuffer>
#include <QDataStream>
#include <TActionContext>
#include <TActionThread>
#include <TAppSettings>
#include <TCache>
#include <TDatabaseContext>
#include <TGlobal>
#include <TWebApplication>
#ifdef Q_OS_LINUX
#include <TActionWorker>
#endif
#ifdef Q_OS_WIN
#include <Windows.h>
#endif
#include <climits>
#include <cstdlib>
#include <random>

constexpr int LZ4_BLOCKSIZE = 1024 * 1024;  // 1 MB

/*!
  Returns a global pointer referring to the unique application object.
*/
TWebApplication *Tf::app() noexcept
{
    return static_cast<TWebApplication *>(qApp);
}

/*!
  Returns a global pointer referring to the unique application settings object.
*/
TAppSettings *Tf::appSettings() noexcept
{
    return TAppSettings::instance();
}

/*!
  Returns the map associated with config file \a configName in 'conf'
  directory.
*/
const QVariantMap &Tf::conf(const QString &configName) noexcept
{
    return Tf::app()->getConfig(configName);
}

/*!
  Causes the current thread to sleep for \a msecs milliseconds.
*/
void Tf::msleep(unsigned long msecs) noexcept
{
    QThread::msleep(msecs);
}

namespace {
std::random_device randev;
std::mt19937 mt(randev());
QMutex mtmtx;
std::mt19937_64 mt64(randev());
QMutex mt64mtx;
}

uint32_t Tf::rand32_r() noexcept
{
    mtmtx.lock();
    uint32_t ret = mt();
    mtmtx.unlock();
    return ret;
}


uint64_t Tf::rand64_r() noexcept
{
    mt64mtx.lock();
    uint64_t ret = mt64();
    mt64mtx.unlock();
    return ret;
}

/*!
  Random number generator in the range from \a min to \a max.
*/
uint64_t Tf::random(uint64_t min, uint64_t max) noexcept
{
    std::uniform_int_distribution<uint64_t> uniform(min, max);
    mt64mtx.lock();
    uint64_t ret = uniform(mt64);
    mt64mtx.unlock();
    return ret;
}

/*!
  Random number generator in the range from 0 to \a max.
*/
uint64_t Tf::random(uint64_t max) noexcept
{
    return random(0, max);
}


TCache *Tf::cache() noexcept
{
    return Tf::currentContext()->cache();
}


TActionContext *Tf::currentContext()
{
    TActionContext *context = nullptr;

    switch (Tf::app()->multiProcessingModule()) {
    case TWebApplication::Thread:
        context = dynamic_cast<TActionThread *>(QThread::currentThread());
        if (Q_LIKELY(context)) {
            return context;
        }
        break;

    case TWebApplication::Epoll:
#ifdef Q_OS_LINUX
        return TActionWorker::instance();
#else
        tFatal("Unsupported MPM: epoll");
#endif
        break;

    default:
        break;
    }

    throw RuntimeException("Can not cast the current thread", __FILE__, __LINE__);
}


TDatabaseContext *Tf::currentDatabaseContext()
{
    TDatabaseContext *context;

    context = TDatabaseContext::currentDatabaseContext();
    if (context) {
        return context;
    }

    context = dynamic_cast<TDatabaseContext *>(QThread::currentThread());
    if (context) {
        return context;
    }

    throw RuntimeException("Can not cast the current thread", __FILE__, __LINE__);
}


QSqlDatabase &Tf::currentSqlDatabase(int id) noexcept
{
    return currentDatabaseContext()->getSqlDatabase(id);
}


QMap<QByteArray, std::function<QObject *()>> *Tf::objectFactories() noexcept
{
    static QMap<QByteArray, std::function<QObject *()>> objectFactoryMap;
    return &objectFactoryMap;
}


QByteArray Tf::lz4Compress(const char *data, int nbytes, int compressionLevel) noexcept
{
    // internal compress function
    auto compress = [](const char *src, int srclen, int level, QByteArray &buffer) {
        const int bufsize = LZ4_compressBound(srclen);
        buffer.reserve(bufsize);

        if (srclen > 0) {
            int rv = LZ4_compress_fast(src, buffer.data(), srclen, bufsize, level);
            if (rv > 0) {
                buffer.resize(rv);
            } else {
                tError("LZ4 compression error: %d", rv);
                buffer.clear();
            }
        } else {
            buffer.resize(0);
        }
    };

    QByteArray ret;
    int rsvsize = LZ4_compressBound(nbytes);
    if (rsvsize < 1) {
        return ret;
    }

    ret.reserve(rsvsize);
    QDataStream dsout(&ret, QIODevice::WriteOnly);
    dsout.setByteOrder(QDataStream::LittleEndian);
    QByteArray buffer;
    int readlen = 0;

    while (readlen < nbytes) {
        int datalen = qMin(nbytes - readlen, LZ4_BLOCKSIZE);
        compress(data + readlen, datalen, compressionLevel, buffer);
        readlen += datalen;

        if (buffer.isEmpty()) {
            ret.clear();
            break;
        } else {
            dsout << (int)buffer.length();
            dsout.writeRawData(buffer.data(), buffer.length());
        }
    }

    return ret;
}


QByteArray Tf::lz4Compress(const QByteArray &data, int compressionLevel) noexcept
{
    return Tf::lz4Compress(data.data(), data.length(), compressionLevel);
}


QByteArray Tf::lz4Uncompress(const char *data, int nbytes) noexcept
{
    QByteArray ret;
    QBuffer srcbuf;
    const int CompressBoundSize = LZ4_compressBound(LZ4_BLOCKSIZE);

    srcbuf.setData(data, nbytes);
    srcbuf.open(QIODevice::ReadOnly);
    QDataStream dsin(&srcbuf);
    dsin.setByteOrder(QDataStream::LittleEndian);

    QByteArray buffer;
    buffer.reserve(LZ4_BLOCKSIZE);

    int readlen = 0;
    while (readlen < nbytes) {
        int srclen;
        dsin >> srclen;
        readlen += sizeof(srclen);

        if (srclen <= 0 || srclen > CompressBoundSize) {
            tError("LZ4 uncompression format error");
            ret.clear();
            break;
        }

        int rv = LZ4_decompress_safe(data + readlen, buffer.data(), srclen, LZ4_BLOCKSIZE);
        dsin.skipRawData(srclen);
        readlen += srclen;

        if (rv > 0) {
            buffer.resize(rv);
            ret += buffer;
        } else {
            tError("LZ4 uncompression error: %d", rv);
            ret.clear();
            break;
        }
    }
    return ret;
}


QByteArray Tf::lz4Uncompress(const QByteArray &data) noexcept
{
    return Tf::lz4Uncompress(data.data(), data.length());
}


qint64 Tf::getMSecsSinceEpoch()
{
    auto p = std::chrono::system_clock::now();  // epoch of system_clock
    return std::chrono::duration_cast<std::chrono::milliseconds>(p.time_since_epoch()).count();
}


/*!
  \def T_EXPORT(VAR)
  Exports the current value of a local variable named \a VAR from the
  controller context to view contexts.
  \see T_FETCH(TYPE,VAR)
 */

/*!
  \def texport(VAR)
  Exports the current value of a local variable named \a VAR from the
  controller context to view contexts.
  \see tfetch(TYPE,VAR)
 */

/*!
  \def T_EXPORT_UNLESS(VAR)
  Exports the current value of a local variable named \a VAR from the
  controller context to view contexts only if a local variable named
  \a VAR isn't exported.
  \see T_EXPORT(VAR)
 */

/*!
  \def texportUnless(VAR)
  Exports the current value of a local variable named \a VAR from the
  controller context to view contexts only if a local variable named
  \a VAR isn't exported.
  \see texport(VAR)
 */

/*!
  \def T_FETCH(TYPE,VAR)
  Creates a local variable named \a VAR with the type \a TYPE on the view and
  fetches the value of a variable named \a VAR exported in a controller context.
  \see T_EXPORT(VAR)
 */

/*!
  \def tfetch(TYPE,VAR)
  Creates a local variable named \a VAR with the type \a TYPE on the view and
  fetches the value of a variable named \a VAR exported in a controller context.
  \see texport(VAR)
 */
