/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tsqldatabase.h"
#include "tsqldriverextension.h"
#include <QCoreApplication>
#include <QMetaObject>
#include <QMetaType>
#include <QtSql>
#include <TSqlObject>
#include <TSqlQuery>
#include <TSystemGlobal>

const QByteArray LockRevision("lock_revision");
const QByteArray CreatedAt("created_at");
const QByteArray UpdatedAt("updated_at");
const QByteArray ModifiedAt("modified_at");

/*!
  \class TSqlObject
  \brief The TSqlObject class is the base class of ORM objects.
  \sa TSqlORMapper
*/

/*!
  Constructor.
 */
TSqlObject::TSqlObject() :
    TModelObject(), QSqlRecord(), sqlError()
{
}

/*!
  Copy constructor.
 */
TSqlObject::TSqlObject(const TSqlObject &other) :
    TModelObject(), QSqlRecord(*static_cast<const QSqlRecord *>(&other)),
    sqlError(other.sqlError)
{
}

/*!
  Assignment operator.
*/
TSqlObject &TSqlObject::operator=(const TSqlObject &other)
{
    QSqlRecord::operator=(*static_cast<const QSqlRecord *>(&other));
    sqlError = other.sqlError;
    return *this;
}

/*!
  Returns the table name, which is generated from the class name.
*/
QString TSqlObject::tableName() const
{
    static const QString ObjectStr = "Object";
    QString tblName;
    QString clsname(metaObject()->className());

    if (Q_LIKELY(clsname.endsWith(ObjectStr))) {
        clsname.resize(clsname.length() - ObjectStr.length());
    }

    tblName.reserve(clsname.length() * 1.2);
    for (int i = 0; i < clsname.length(); ++i) {
        if (i > 0 && clsname.at(i).isUpper()) {
            tblName += QLatin1Char('_');
        }
        tblName += clsname.at(i).toLower();
    }
    return tblName;
}

/*!
  \fn virtual int TSqlObject::primaryKeyIndex() const
  Returns the position of the primary key field on the table.
  This is a virtual function.
*/

/*!
  \fn virtual int TSqlObject::autoValueIndex() const
  Returns the position of the auto-generated value field on
  the table. This is a virtual function.
*/

/*!
  \fn virtual int TSqlObject::databaseId() const
  Returns the database ID.
*/

/*!
  \fn bool TSqlObject::isNull() const
  Returns true if there is no database record associated with the
  object; otherwise returns false.
*/

/*!
  \fn bool TSqlObject::isNew() const
  Returns true if it is a new object, otherwise returns false.
  Equivalent to isNull().
*/

/*!
  \fn QSqlError TSqlObject::error() const
  Returns a QSqlError object which contains information about
  the last error that occurred on the database.
*/

/*!
  Sets the \a record. This function is for internal use only.
*/
void TSqlObject::setRecord(const QSqlRecord &record, const QSqlError &error)
{
    QSqlRecord::operator=(record);
    syncToObject();
    sqlError = error;
}

/*!
  Inserts new record into the database, based on the current properties
  of the object.
*/
bool TSqlObject::create()
{
    // Sets the values of 'created_at', 'updated_at' or 'modified_at' properties
    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i) {
        const char *propName = metaObject()->property(i).name();
        QByteArray prop = QByteArray(propName).toLower();

        if (Tf::strcmp(prop, CreatedAt) || Tf::strcmp(prop, UpdatedAt) || Tf::strcmp(prop, ModifiedAt)) {
            setProperty(propName, QDateTime::currentDateTime());
        } else if (Tf::strcmp(prop, LockRevision)) {
            // Sets the default value of 'revision' property
            setProperty(propName, 1);  // 1 : default value
        } else {
            // do nothing
        }
    }

    syncToSqlRecord();

    QString autoValName;
    QSqlRecord record = *this;
    if (autoValueIndex() >= 0) {
        autoValName = field(autoValueIndex()).name();
        record.remove(autoValueIndex());  // not insert the value of auto-value field
    }

    auto &database = getDatabase();
    QString ins, values;
    ins.reserve(511);
    values.reserve(255);

    ins += QLatin1String("INSERT INTO ");
    ins += TSqlQuery::escapeIdentifier(tableName(), QSqlDriver::TableName, database.driver());
    ins += QLatin1String(" (");

    int autoidx = metaObject()->propertyOffset() + autoValueIndex();
    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i) {
        auto metaProp = metaObject()->property(i);
        const char *propName = metaProp.name();
        QVariant val = QObject::property(propName);

        if (i != autoidx) {
            ins += TSqlQuery::escapeIdentifier(QLatin1String(propName), QSqlDriver::FieldName, database.driver());
            ins += QLatin1Char(',');
            values += TSqlQuery::formatValue(val, metaProp.metaType(), database);
            values += QLatin1Char(',');
        }
    }

    ins.chop(1);
    ins += QLatin1String(") VALUES (");
    values.chop(1);
    ins += values;
    ins += QLatin1Char(')');

    TSqlQuery query(database);
    bool ret = query.exec(ins);
    sqlError = query.lastError();
    if (Q_LIKELY(ret)) {
        // Gets the last inserted value of auto-value field
        if (autoValueIndex() >= 0) {
            QVariant lastid = query.lastInsertId();

            if (!lastid.isValid() && database.driver()->dbmsType() == QSqlDriver::PostgreSQL) {
                // For PostgreSQL without OIDS
                ret = query.exec(QStringLiteral("SELECT LASTVAL()"));
                sqlError = query.lastError();
                if (Q_LIKELY(ret)) {
                    lastid = query.getNextValue();
                }
            }

            if (lastid.isValid()) {
                QObject::setProperty(autoValName.toLatin1().constData(), lastid);
                QSqlRecord::setValue(autoValueIndex(), lastid);
            }
        }
    }
    return ret;
}

/*!
  Updates the corresponding record with the properties of the object.
*/
bool TSqlObject::update()
{
    if (isNew()) {
        sqlError = QSqlError(QLatin1String("No record to update"),
            QString(), QSqlError::UnknownError);
        Tf::warn("Unable to update the '{}' object. Create it before!", metaObject()->className());
        return false;
    }

    auto &database = getDatabase();
    QString where;
    where.reserve(255);
    where.append(QLatin1String(" WHERE "));

    // Updates the value of 'updated_at' or 'modified_at' property
    bool updflag = false;
    int revIndex = -1;
    const QMetaType intMetaType(QMetaType::Int);

    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i) {
        const char *propName = metaObject()->property(i).name();
        QByteArray prop = QByteArray(propName).toLower();

        if (!updflag && (Tf::strcmp(prop, UpdatedAt) || Tf::strcmp(prop, ModifiedAt))) {
            setProperty(propName, QDateTime::currentDateTime());
            updflag = true;

        } else if (revIndex < 0 && Tf::strcmp(prop, LockRevision)) {
            bool ok;
            int oldRevision = property(propName).toInt(&ok);

            if (!ok || oldRevision <= 0) {
                sqlError = QSqlError(QLatin1String("Unable to convert the 'revision' property to an int"),
                    QString(), QSqlError::UnknownError);
                Tf::error("Unable to convert the 'revision' property to an int, {}", objectName());
                return false;
            }

            setProperty(propName, oldRevision + 1);
            revIndex = i;

            where.append(QLatin1String(propName));
            where.append(QLatin1Char('=')).append(TSqlQuery::formatValue(oldRevision, intMetaType, database));
            where.append(QLatin1String(" AND "));
        } else {
            // continue
        }
    }

    QString upd;  // UPDATE Statement
    upd.reserve(512);
    upd.append(QLatin1String("UPDATE "));
    upd.append(TSqlQuery::escapeIdentifier(tableName(), QSqlDriver::TableName, database.driver()));
    upd.append(QLatin1String(" SET "));

    int pkidx = metaObject()->propertyOffset() + primaryKeyIndex();
    QMetaProperty metaProp = metaObject()->property(pkidx);
    const char *pkName = metaProp.name();
    if (primaryKeyIndex() < 0 || !pkName) {
        QString msg = QString("Primary key not found for table ") + tableName() + QLatin1String(". Create a primary key!");
        sqlError = QSqlError(msg, QString(), QSqlError::StatementError);
        Tf::error("{}", msg);
        return false;
    }

    auto pkType = metaProp.metaType();
    QVariant origpkval = value(pkName);
    where.append(QLatin1String(pkName));
    where.append(QLatin1Char('=')).append(TSqlQuery::formatValue(origpkval, pkType, database));
    // Restore the value of primary key
    QObject::setProperty(pkName, origpkval);

    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i) {
        metaProp = metaObject()->property(i);
        const char *propName = metaProp.name();
        QVariant newval = QObject::property(propName);
        QVariant recval = QSqlRecord::value(QLatin1String(propName));
        if (i != pkidx && recval.isValid() && recval != newval) {
            upd.append(TSqlQuery::escapeIdentifier(QLatin1String(propName), QSqlDriver::FieldName, database.driver()));
            upd.append(QLatin1Char('='));
            upd.append(TSqlQuery::formatValue(newval, metaProp.metaType(), database));
            upd.append(QLatin1Char(','));
        }
    }

    if (!upd.endsWith(QLatin1Char(','))) {
        tSystemDebug("SQL UPDATE: Same values as that of the record. No need to update.");
        return true;
    }

    upd.chop(1);
    syncToSqlRecord();
    upd.append(where);

    TSqlQuery query(database);
    bool ret = query.exec(upd);
    sqlError = query.lastError();
    if (ret) {
        // Optimistic lock check
        if (revIndex >= 0 && query.numRowsAffected() != 1) {
            QString msg = QString("Row was updated or deleted from table ") + tableName() + QLatin1String(" by another transaction");
            sqlError = QSqlError(msg, QString(), QSqlError::UnknownError);
            throw SqlException(msg, __FILE__, __LINE__);
        }
    }
    return ret;
}

/*!
  Depending on whether condition matches, inserts new record or updates
  the corresponding record with the properties of the object. If possible,
  invokes UPSERT in relational database.
*/
bool TSqlObject::save()
{
    auto &sqldb = getDatabase();
    const auto &db = TSqlDatabase::database(sqldb.connectionName());
    QString lockrev;

    if (!db.isUpsertSupported() || !db.isUpsertEnabled()) {
        return (isNew()) ? create() : update();
    }

    // Sets the values of 'created_at', 'updated_at' or 'modified_at' properties
    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i) {
        const char *propName = metaObject()->property(i).name();
        QByteArray prop = QByteArray(propName).toLower();

        if (Tf::strcmp(prop, CreatedAt) || Tf::strcmp(prop, UpdatedAt) || Tf::strcmp(prop, ModifiedAt)) {
            setProperty(propName, QDateTime::currentDateTime());
        } else if (Tf::strcmp(prop, LockRevision)) {
            // Sets the default value of 'revision' property
            setProperty(propName, 1);  // 1 : default value
            lockrev = LockRevision;
        } else {
            // do nothing
        }
    }

    syncToSqlRecord();

    QSqlRecord recordToInsert = *this;
    QSqlRecord recordToUpdate = *this;
    QList<int> removeFields;
    QString autoValName;

    if (autoValueIndex() >= 0 && autoValueIndex() != primaryKeyIndex()) {
        autoValName = field(autoValueIndex()).name();
        recordToInsert.remove(autoValueIndex());  // not insert the value of auto-value field
    }

    int idxtmp;
    if ((idxtmp = recordToUpdate.indexOf(CreatedAt)) >= 0) {
        recordToUpdate.remove(idxtmp);
    }
    if ((idxtmp = recordToUpdate.indexOf(LockRevision)) >= 0) {
        recordToUpdate.remove(idxtmp);
    }

    QString upst = db.driverExtension()->upsertStatement(tableName(), recordToInsert, recordToUpdate, field(primaryKeyIndex()).name(), lockrev);
    if (upst.isEmpty()) {
        // In case unable to generate upsert statement
        return (isNew()) ? create() : update();
    }

    TSqlQuery query(sqldb);
    bool ret = query.exec(upst);
    sqlError = query.lastError();
    if (ret) {
        // Gets the last inserted value of auto-value field
        if (autoValueIndex() >= 0) {
            QVariant lastid = query.lastInsertId();
            if (lastid.isValid()) {
                QObject::setProperty(autoValName.toLatin1().constData(), lastid);
                QSqlRecord::setValue(autoValueIndex(), lastid);
            }
        }
    }
    return ret;
}

/*!
  Deletes the record with this primary key from the database.
*/
bool TSqlObject::remove()
{
    if (isNew()) {
        sqlError = QSqlError(QLatin1String("No record to remove"),
            QString(), QSqlError::UnknownError);
        Tf::warn("Unable to remove the '{}' object. Create it before!", metaObject()->className());
        return false;
    }

    QSqlDatabase &database = getDatabase();
    QString del = database.driver()->sqlStatement(QSqlDriver::DeleteStatement, tableName(), *static_cast<QSqlRecord *>(this), false);
    if (del.isEmpty()) {
        sqlError = QSqlError(QLatin1String("Unable to delete row"),
            QString(), QSqlError::StatementError);
        return false;
    }

    del.append(QLatin1String(" WHERE "));
    int revIndex = -1;
    const QMetaType intMetaType(QMetaType::Int);

    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i) {
        const char *propName = metaObject()->property(i).name();
        QByteArray prop = QByteArray(propName).toLower();

        if (Tf::strcmp(prop, LockRevision)) {
            bool ok;
            int revision = property(propName).toInt(&ok);

            if (!ok || revision <= 0) {
                sqlError = QSqlError(QLatin1String("Unable to convert the 'revision' property to an int"),
                    QString(), QSqlError::UnknownError);
                Tf::error("Unable to convert the 'revision' property to an int, {}", objectName());
                return false;
            }

            del.append(QLatin1String(propName));
            del.append(QLatin1Char('=')).append(TSqlQuery::formatValue(revision, intMetaType, database));
            del.append(QLatin1String(" AND "));

            revIndex = i;
            break;
        }
    }

    auto metaProp = metaObject()->property(metaObject()->propertyOffset() + primaryKeyIndex());
    const char *pkName = metaProp.name();
    if (primaryKeyIndex() < 0 || !pkName) {
        QString msg = QString("Primary key not found for table ") + tableName() + QLatin1String(". Create a primary key!");
        sqlError = QSqlError(msg, QString(), QSqlError::StatementError);
        Tf::error("{}", msg);
        return false;
    }
    del.append(QLatin1String(pkName));
    auto metaType = metaProp.metaType();
    del.append(QLatin1Char('=')).append(TSqlQuery::formatValue(value(pkName), metaType, database));

    TSqlQuery query(database);
    bool ret = query.exec(del);
    sqlError = query.lastError();
    if (ret) {
        // Optimistic lock check
        if (query.numRowsAffected() != 1) {
            if (revIndex >= 0) {
                QString msg = QString("Row was updated or deleted from table ") + tableName() + QLatin1String(" by another transaction");
                sqlError = QSqlError(msg, QString(), QSqlError::UnknownError);
                throw SqlException(msg, __FILE__, __LINE__);
            }
            Tf::warn("Row was deleted by another transaction, {}", tableName());
        }
        clear();
    }
    return ret;
}

/*!
  Reloads the values of  the record onto the properties.
 */
bool TSqlObject::reload()
{
    if (isEmpty()) {
        return false;
    }
    syncToObject();
    return true;
}

/*!
  Returns true if the values of the properties differ with the record on the
  database; otherwise returns false.
 */
bool TSqlObject::isModified() const
{
    if (isNew())
        return false;

    for (int i = 0; i < QSqlRecord::count(); ++i) {
        QString name = field(i).name();
        int index = metaObject()->indexOfProperty(name.toLatin1().constData());
        if (index >= 0) {
            if (value(name) != property(name.toLatin1().constData())) {
                return true;
            }
        }
    }
    return false;
}

/*!
  Synchronizes the internal record data to the properties of the object.
  This function is for internal use only.
*/
void TSqlObject::syncToObject()
{
    int offset = metaObject()->propertyOffset();
    for (int i = 0; i < QSqlRecord::count(); ++i) {
        QString propertyName = field(i).name();
        QByteArray name = propertyName.toLatin1();
        int index = metaObject()->indexOfProperty(name.constData());
        if (index >= offset) {
            QObject::setProperty(name.constData(), value(propertyName));
        }
    }
}

/*!
  Synchronizes the properties to the internal record data.
  This function is for internal use only.
*/
void TSqlObject::syncToSqlRecord()
{
    auto &db = getDatabase();
    QSqlRecord::operator=(db.record(tableName()));
    const QMetaObject *metaObj = metaObject();
    for (int i = metaObj->propertyOffset(); i < metaObj->propertyCount(); ++i) {
        const char *propName = metaObj->property(i).name();
        int idx = indexOf(propName);
        if (idx >= 0) {
            QSqlRecord::setValue(idx, QObject::property(propName));
        } else {
            Tf::warn("invalid name: {}", propName);
        }
    }
}


QSqlDatabase &TSqlObject::getDatabase()
{
    if (!_database.isValid()) {
        _database = Tf::currentSqlDatabase(databaseId());
    }
    return _database;
}
