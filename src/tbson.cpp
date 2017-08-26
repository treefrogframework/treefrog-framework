/* Copyright (c) 2012-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TBson>
#include <QDateTime>
#include <QRegExp>
#include <QStringList>
#include <QCoreApplication>
#include <QtEndian>
#include <atomic>
extern "C" {
#include "mongoc.h"
}

/*!
  \class TBson
  \brief The TBson class represents a Binary JSON for MongoDB.
*/

TBson::TBson()
    : bsonData(bson_new())
{ }


TBson::~TBson()
{
    bson_destroy(bsonData);
}


TBson::TBson(const TBson &other)
{
    bsonData = bson_copy(other.bsonData);
}


TBson::TBson(const TBsonObject *bson)
{
    bsonData = (bson) ? bson_copy((const bson_t *)bson) : bson_new();
}


QVariant TBson::value(const QString &key, const QVariant &defaultValue) const
{
    return fromBson(*this).value(key, defaultValue);
}


QVariantMap TBson::fromBson(const TBson &bson)
{
    return TBson::fromBson(bson.constData());
}


QVariantMap TBson::fromBson(const TBsonObject *obj)
{
    QVariantMap ret;
    bson_iter_t it;
    const bson_t *bson = (const bson_t *)obj;

    bson_iter_init(&it, bson);
    while (bson_iter_next(&it)) {
        bson_type_t t = bson_iter_type(&it);
        QString key(bson_iter_key(&it));

        switch (t) {
        case BSON_TYPE_EOD:
            return ret;
            break;

        case BSON_TYPE_DOUBLE:
            ret[key] = bson_iter_double(&it);
            break;

        case BSON_TYPE_UTF8:
            ret[key] = QString::fromUtf8(bson_iter_utf8(&it, nullptr));
            break;

        case BSON_TYPE_ARRAY: {
            const uint8_t *docbuf = nullptr;
            uint32_t doclen = 0;
            bson_t sub[1];

            bson_iter_array(&it, &doclen, &docbuf);
            if (bson_init_static(sub, docbuf, doclen)) {
                ret[key] = fromBson(sub).values();
            }
            break; }

        case BSON_TYPE_DOCUMENT: {
            const uint8_t *docbuf = nullptr;
            uint32_t doclen = 0;
            bson_t sub[1];

            bson_iter_document(&it, &doclen, &docbuf);
            if (bson_init_static(sub, docbuf, doclen)) {
                ret[key] = fromBson(sub);
            }
            break; }

        case BSON_TYPE_BINARY: {
            const uint8_t *binary = nullptr;
            bson_subtype_t subtype = BSON_SUBTYPE_BINARY;
            uint32_t len = 0;

            bson_iter_binary(&it, &subtype, &len, &binary);
            if (binary) {
                ret[key] = QByteArray((char *)binary, len);
            }
            break; }

        case BSON_TYPE_UNDEFINED:
            ret[key] = QVariant();
            break;

        case BSON_TYPE_OID: {
            char oidhex[25];
            bson_oid_to_string(bson_iter_oid(&it), oidhex);
            ret[key] = QString(oidhex);
            break; }

        case BSON_TYPE_BOOL:
            ret[key] = (bool)bson_iter_bool(&it);
            break;

        case BSON_TYPE_DATE_TIME: {
#if QT_VERSION >= 0x040700
            QDateTime date;
            date.setMSecsSinceEpoch(bson_iter_date_time(&it));
#else
            qint64 val = bson_iter_date_time(&it);
            qint64 days = val / 86400000;  // 24*60*60*1000
            int msecs = val % 86400000;
            QDate dt = QDate(1970, 1, 1).addDays(days);
            QTime tm = QTime(0, 0, 0).addMSecs(msecs);
            QDateTime date(dt, tm, Qt::UTC);
#endif
            ret[key] = date;
            break; }

        case BSON_TYPE_NULL:
            ret[key] = QVariant();
            break;

        case BSON_TYPE_REGEX:
            ret[key] = QRegExp(QLatin1String(bson_iter_regex(&it, nullptr)));
            break;

        case BSON_TYPE_CODE:
            ret[key] = QString(bson_iter_code(&it, nullptr));
            break;

        case BSON_TYPE_SYMBOL:
            ret[key] = QString(bson_iter_symbol(&it, nullptr));
            break;

        case BSON_TYPE_INT32:
            ret[key] = bson_iter_int32(&it);
            break;

        case BSON_TYPE_INT64:
            ret[key] = (qint64)bson_iter_int64(&it);
            break;

        case BSON_TYPE_CODEWSCOPE: // FALL THROUGH
        case BSON_TYPE_TIMESTAMP:  // FALL THROUGH (internal use)
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


static bool appendBsonValue(bson_t *bson, const QString &key, const QVariant &value)
{
    const QLatin1String oidkey("_id");
    bool ok = true;
    int type = value.type();

    // _id
    if (key == oidkey) {
        QByteArray oidVal = value.toByteArray();
        if (oidVal.length() == 24) {
            // ObjectId
            bson_oid_t oid;
            bson_oid_init_from_string(&oid, oidVal.data());
            BSON_APPEND_OID(bson, oidkey.latin1(), &oid);
        } else {
            int id = value.toInt(&ok);
            if (ok) {
                BSON_APPEND_INT32(bson, oidkey.latin1(), id);
            } else {
                BSON_APPEND_UTF8(bson, oidkey.latin1(), value.toString().toUtf8().data());
            }
        }
        return true;
    }

    switch (type) {
    case QVariant::Int:
        BSON_APPEND_INT32(bson, qPrintable(key), value.toInt(&ok));
        break;

    case QVariant::String:
        BSON_APPEND_UTF8(bson, qPrintable(key), value.toString().toUtf8().data());
        break;

    case QVariant::LongLong:
        BSON_APPEND_INT64(bson, qPrintable(key), value.toLongLong(&ok));
        break;

    case QVariant::Map:
        BSON_APPEND_DOCUMENT(bson, qPrintable(key), (const bson_t *)TBson::toBson(value.toMap()).constData());
        break;

    case QVariant::Double:
        BSON_APPEND_DOUBLE(bson, qPrintable(key), value.toDouble(&ok));
        break;

    case QVariant::Bool:
        BSON_APPEND_BOOL(bson, qPrintable(key), value.toBool());
        break;

    case QVariant::DateTime: {
#if QT_VERSION >= 0x040700
        BSON_APPEND_DATE_TIME(bson, qPrintable(key), value.toDateTime().toMSecsSinceEpoch());
#else
        QDateTime utcDate = value.toDateTime().toUTC();
        qint64 ms = utcDate.time().msec();
        qint64 tm = utcDate.toTime_t() * 1000LL;
        if (ms > 0) {
          tm += ms;
        }
        BSON_APPEND_DATE_TIME(bson, qPrintable(key), tm);
#endif
        break; }

    case QVariant::ByteArray: {
        QByteArray ba = value.toByteArray();
        BSON_APPEND_BINARY(bson, qPrintable(key), BSON_SUBTYPE_BINARY, (uint8_t *)ba.constData(), ba.length());
        break; }

    case QVariant::List:  // FALL THROUGH
    case QVariant::StringList: {
        bson_t child;
        BSON_APPEND_ARRAY_BEGIN(bson, qPrintable(key), &child);

        int i = 0;
        for (auto &var : (const QList<QVariant>&)value.toList()) {
            appendBsonValue(&child, QString::number(i++), var);
        }
        bson_append_array_end(bson, &child);
        break; }

    case QVariant::Invalid:
        BSON_APPEND_UNDEFINED(bson, qPrintable(key));
        break;

    default:
        tError("toBson() failed to convert  name:%s  type:%d", qPrintable(key), type);
        ok = false;
        break;
    }
    return ok;
}


static void appendBson(TBsonObject *bson, const QVariantMap &map)
{
    for (QMapIterator<QString, QVariant> it(map); it.hasNext(); ) {
        const QVariant &val = it.next().value();
        bool res = appendBsonValue((bson_t *)bson, qPrintable(it.key()), val);
        if (!res)
            break;
    }
}


TBson TBson::toBson(const QVariantMap &map)
{
    TBson ret;
    appendBson(ret.data(), map);
    return ret;
}


TBson TBson::toBson(const QVariantMap &query, const QVariantMap &orderBy)
{
    TBson ret;
    bson_t child;

    // query clause
    BSON_APPEND_DOCUMENT_BEGIN((bson_t *)ret.data(), "$query", &child);
    appendBson(&child, query);
    bson_append_document_end((bson_t *)ret.data(), &child);

    // orderBy clause
    if (!orderBy.isEmpty()) {
        BSON_APPEND_DOCUMENT_BEGIN((bson_t *)ret.data(), "$orderby", &child);
        appendBson(&child, orderBy);
        bson_append_document_end((bson_t *)ret.data(), &child);
    }
    return ret;
}


TBson TBson::toBson(const QStringList &lst)
{
    TBson ret;
    for (auto &str : lst) {
        bool res = appendBsonValue((bson_t *)ret.data(), qPrintable(str), 1);
        if (!res)
            break;
    }
    return ret;
}


QString TBson::generateObjectId()
{
    bson_oid_t oid;
    bson_oid_init(&oid, nullptr);

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

/* BSON dump
    auto str = bson_as_json(bson, nullptr);
    printf("%s\n", str);
    bson_free(str);
*/
