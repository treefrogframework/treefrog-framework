/* Copyright (c) 2012-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TBson>
#include <QDateTime>
#include <QRegExp>
#include <QStringList>
#include <QAtomicInt>
#include <QCoreApplication>
#include <QtEndian>
#include "mongo.h"

/*!
  \class TBson
  \brief The TBson class represents a Binary JSON for MongoDB.
*/

TBson::TBson()
    : bsonData(new bson)
{
    bson_init((bson *)bsonData);
}


TBson::~TBson()
{
    bson_destroy((bson *)bsonData);
    delete (bson *)bsonData;
}


TBson::TBson(const TBson &other)
{
    bson_copy((bson *)bsonData, (bson *)other.bsonData);
}


QVariantMap TBson::fromBson(const TBson &bs)
{
    return TBson::fromBson(bs.constData());
}


QVariantMap TBson::fromBson(const TBsonObject *obj)
{
    QVariantMap ret;
    bson_iterator it[1];

    bson_iterator_init(it, (const bson *)obj);
    while (bson_iterator_more(it)) {
        bson_type t = bson_iterator_next(it);
        QString key(bson_iterator_key(it));

        switch (t) {
        case BSON_EOO:
            return ret;
            break;

        case BSON_DOUBLE:
            ret[key] = bson_iterator_double(it);
            break;

        case BSON_STRING:
            ret[key] = QString(bson_iterator_string(it));
            break;

        case BSON_ARRAY: {
            bson sub[1];
            bson_iterator_subobject_init(it, sub, (bson_bool_t)true);
            ret[key] = fromBson(sub).values();
            break; }

        case BSON_OBJECT: {
            bson sub[1];
            bson_iterator_subobject_init(it, sub, (bson_bool_t)true);
            ret[key] = fromBson(sub);
            break; }

        case BSON_BINDATA: {
            int len = bson_iterator_bin_len(it);
            ret[key] = QByteArray(bson_iterator_bin_data(it), len);
            break; }

        case BSON_UNDEFINED:
            ret[key] = QVariant();
            break;

        case BSON_OID: {
            char oidhex[25];
            bson_oid_to_string(bson_iterator_oid(it), oidhex);
            ret[key] = QString(oidhex);
            break; }

        case BSON_BOOL:
            ret[key] = (bool)bson_iterator_bool(it);
            break;

        case BSON_DATE: {
#if QT_VERSION >= 0x040700
            QDateTime date;
            date.setMSecsSinceEpoch(bson_iterator_date(it));
#else
            qint64 val = bson_iterator_date(it);
            qint64 days = val / 86400000;  // 24*60*60*1000
            int msecs = val % 86400000;
            QDate dt = QDate(1970, 1, 1).addDays(days);
            QTime tm = QTime(0, 0, 0).addMSecs(msecs);
            QDateTime date(dt, tm, Qt::UTC);
#endif
            ret[key] = date;
            break; }

        case BSON_NULL:
            ret[key] = QVariant();
            break;

        case BSON_REGEX:
            ret[key] = QRegExp(QLatin1String(bson_iterator_regex(it)));
            break;

        case BSON_DBREF: // Deprecated
            break;

        case BSON_CODE:
            ret[key] = QString(bson_iterator_code(it));
            break;

        case BSON_SYMBOL:
            ret[key] = QString(bson_iterator_string(it));
            break;

        case BSON_INT:
            ret[key] = bson_iterator_int(it);
            break;

        case BSON_LONG:
            ret[key] = (qint64)bson_iterator_long(it);
            break;

        case BSON_CODEWSCOPE: // FALL THROUGH
        case BSON_TIMESTAMP:  // FALL THROUGH (internal use)
            // do nothing
            break;

        default:
            tError("fromBson() unknown type: %d", t);
            break;
        }
        //tSystemDebug("fromBson : t:%d key:%s = %s", t, qPrintable(key), qPrintable(ret[key].toString()));
    }
    return ret;
}


static bool appendBsonValue(bson *b, const QString &key, const QVariant &value)
{
    const QLatin1String oidkey("_id");
    bool ok = true;
    int type = value.type();

    switch (type) {
    case QVariant::Int:
        bson_append_int(b, qPrintable(key), value.toInt(&ok));
        break;

    case QVariant::String:
        if (key == oidkey) {
            // OID
            bson_oid_t oid;
            bson_oid_from_string(&oid, qPrintable(value.toString()));
            bson_append_oid(b, oidkey.latin1(), &oid);
        } else {
            bson_append_string(b, qPrintable(key), qPrintable(value.toString()));
        }
        break;

    case QVariant::LongLong:
        bson_append_long(b, qPrintable(key), value.toLongLong(&ok));
        break;

    case QVariant::Map:
        bson_append_bson(b, qPrintable(key), (const bson *)TBson::toBson(value.toMap()).constData());
        break;

    case QVariant::Double:
        bson_append_double(b, qPrintable(key), value.toDouble(&ok));
        break;

    case QVariant::Bool:
        bson_append_bool(b, qPrintable(key), value.toBool());
        break;

    case QVariant::DateTime: {
#if QT_VERSION >= 0x040700
        bson_append_date(b, qPrintable(key), value.toDateTime().toMSecsSinceEpoch());
#else
        QDateTime utcDate = value.toDateTime().toUTC();
        qint64 ms = utcDate.time().msec();
        qint64 tm = utcDate.toTime_t() * 1000LL;
        if (ms > 0) {
          tm += ms;
        }
        bson_append_date(b, qPrintable(key), tm);
#endif
        break; }

    case QVariant::ByteArray: {
        QByteArray ba = value.toByteArray();
        bson_append_binary(b, qPrintable(key), BSON_BIN_BINARY, ba.constData(), ba.length());
        break; }

    case QVariant::List:  // FALL THROUGH
    case QVariant::StringList: {
        bson_append_start_array(b, qPrintable(key));
        QVariantList lst = value.toList();

        int i = 0;
        for (QListIterator<QVariant> it(lst); it.hasNext(); ) {
            const QVariant &var = it.next();
            appendBsonValue(b, QString::number(i++), var);
        }

        bson_append_finish_array(b);
        break; }

    case QVariant::Invalid:
        bson_append_undefined(b,  qPrintable(key));
        break;

    default:
        tError("toBson() failed to convert  name:%s  type:%d", qPrintable(key), type);
        ok = false;
        break;
    }
    return ok;
}


static void appendBson(TBsonObject *bs, const QVariantMap &map)
{
    for (QMapIterator<QString, QVariant> it(map); it.hasNext(); ) {
        const QVariant &val = it.next().value();
        bool res = appendBsonValue((bson *)bs, qPrintable(it.key()), val);
        if (!res)
            break;
    }
}


TBson TBson::toBson(const QVariantMap &map)
{
    TBson ret;
    appendBson(ret.data(), map);
    bson_finish((bson *)ret.data());
    return ret;
}


TBson TBson::toBson(const QVariantMap &query, const QVariantMap &orderBy)
{
    TBson ret;

    // query clause
    bson_append_start_object((bson *)ret.data(), "$query");
    appendBson(ret.data(), query);
    bson_append_finish_object((bson *)ret.data());

    // orderBy clause
    if (!orderBy.isEmpty()) {
        bson_append_start_object((bson *)ret.data(), "$orderby");
        appendBson(ret.data(), orderBy);
        bson_append_finish_object((bson *)ret.data());
    }

    bson_finish((bson *)ret.data());
    return ret;
}


TBson TBson::toBson(const QStringList &lst)
{
    TBson ret;

    for (QStringListIterator it(lst); it.hasNext(); ) {
        const QString &str = it.next();

        bool res = appendBsonValue((bson *)ret.data(), qPrintable(str), 1);
        if (!res)
            break;
    }

    bson_finish((bson *)ret.data());
    return ret;
}


static inline int oidFuzz()
{
    static int machineId = Tf::randXor128();
    int pid = QCoreApplication::applicationPid();
    return qToBigEndian((machineId << 8) | ((pid & 0xFF00) >> 8));
}


static inline int oidInc()
{
    static QAtomicInt incr = Tf::randXor128();
    int pid = QCoreApplication::applicationPid();
    return ((pid & 0xFF) << 24) | ((int)incr.fetchAndAddOrdered(1) & 0xFFFFFF);
}


QString TBson::generateObjectId()
{
    static bool once = false;

    if (!once) {
        bson_set_oid_fuzz(oidFuzz);
        bson_set_oid_inc(oidInc);
        once = true;
    }

    bson_oid_t oid;
    bson_oid_gen(&oid);

    QByteArray oidhex = QByteArray((char *)&oid, sizeof(oid)).toHex();
    return QLatin1String(oidhex.data());
}

/*
  ObjectId is a 12-byte BSON type, constructed using:
    a 4-byte value representing the seconds since the Unix epoch,
    a 3-byte machine identifier,
    a 2-byte process id, and
    a 3-byte counter, starting with a random value.
*/
