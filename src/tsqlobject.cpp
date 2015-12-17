/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtSql>
#include <QCoreApplication>
#include <QMetaObject>
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
TSqlObject::TSqlObject()
    : TModelObject(), QSqlRecord(), sqlError()
{ }

/*!
  Copy constructor.
 */
TSqlObject::TSqlObject(const TSqlObject &other)
    : TModelObject(), QSqlRecord(*static_cast<const QSqlRecord *>(&other)),
      sqlError(other.sqlError)
{ }

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
    QString tblName;
    QString clsname(metaObject()->className());

    for (int i = 0; i < clsname.length(); ++i) {
        if (i > 0 && clsname[i].isUpper()) {
            tblName += '_';
        }
        tblName += clsname[i].toLower();
    }
    tblName.remove(QRegExp("_object$"));
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

        if (prop == CreatedAt || prop == UpdatedAt || prop == ModifiedAt) {
            setProperty(propName, QDateTime::currentDateTime());
        } else if (prop == LockRevision) {
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
        record.remove(autoValueIndex()); // not insert the value of auto-value field
    }

    QSqlDatabase &database = Tf::currentSqlDatabase(databaseId());
    QString ins = database.driver()->sqlStatement(QSqlDriver::InsertStatement, tableName(), record, false);
    if (Q_UNLIKELY(ins.isEmpty())) {
        sqlError = QSqlError(QLatin1String("No fields to insert"),
                             QString(), QSqlError::StatementError);
        tWarn("SQL statement error, no fields to insert");
        return false;
    }

    TSqlQuery query(database);
    bool ret = query.exec(ins);
    sqlError = query.lastError();
    if (Q_LIKELY(ret)) {
        // Gets the last inserted value of auto-value field
        if (autoValueIndex() >= 0) {
            QVariant lastid = query.lastInsertId();

            if (!lastid.isValid() && database.driverName().toUpper() == QLatin1String("QPSQL")) {
                // For PostgreSQL without OIDS
                ret = query.exec("SELECT LASTVAL()");
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
        tWarn("Unable to update the '%s' object. Create it before!", metaObject()->className());
        return false;
    }

    QSqlDatabase &database = Tf::currentSqlDatabase(databaseId());
    QString where(" WHERE ");

    // Updates the value of 'updated_at' or 'modified_at' property
    bool updflag = false;
    int revIndex = -1;

    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i) {
        const char *propName = metaObject()->property(i).name();
        QByteArray prop = QByteArray(propName).toLower();

        if (!updflag && (prop == UpdatedAt || prop == ModifiedAt)) {
            setProperty(propName, QDateTime::currentDateTime());
            updflag = true;

        } else if (revIndex < 0 && prop == LockRevision) {
            bool ok;
            int oldRevision = property(propName).toInt(&ok);

            if (!ok || oldRevision <= 0) {
                sqlError = QSqlError(QLatin1String("Unable to convert the 'revision' property to an int"),
                                     QString(), QSqlError::UnknownError);
                tError("Unable to convert the 'revision' property to an int, %s", qPrintable(objectName()));
                return false;
            }

            setProperty(propName, oldRevision + 1);
            revIndex = i;

            where.append(QLatin1String(propName));
            where.append("=").append(TSqlQuery::formatValue(oldRevision, database));
            where.append(" AND ");
        } else {
            // continue
        }
    }

    QString upd;   // UPDATE Statement
    upd.reserve(255);
    upd.append(QLatin1String("UPDATE ")).append(tableName()).append(QLatin1String(" SET "));

    int pkidx = metaObject()->propertyOffset() + primaryKeyIndex();
    const char *pkName = metaObject()->property(pkidx).name();
    if (primaryKeyIndex() < 0 || !pkName) {
        QString msg = QString("Primary key not found for table ") + tableName() + QLatin1String(". Create a primary key!");
        sqlError = QSqlError(msg, QString(), QSqlError::StatementError);
        tError("%s", qPrintable(msg));
        return false;
    }

    QVariant origpkval = value(pkName);
    where.append(QLatin1String(pkName));
    where.append("=").append(TSqlQuery::formatValue(origpkval, database));
    // Restore the value of primary key
    QObject::setProperty(pkName, origpkval);

    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i) {
        const char *propName = metaObject()->property(i).name();
        QVariant newval = QObject::property(propName);
        QVariant recval = QSqlRecord::value(QLatin1String(propName));
        if (i != pkidx && recval.isValid() && recval != newval) {
            upd.append(QLatin1String(propName));
            upd.append(QLatin1Char('='));
            upd.append(TSqlQuery::formatValue(newval, database));
            upd.append(QLatin1String(", "));
        }
    }

    if (!upd.endsWith(QLatin1String(", "))) {
        tSystemDebug("SQL UPDATE: Same values as that of the record. No need to update.");
        return true;
    }

    upd.chop(2);
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
  Deletes the record with this primary key from the database.
*/
bool TSqlObject::remove()
{
    if (isNew()) {
        sqlError = QSqlError(QLatin1String("No record to remove"),
                             QString(), QSqlError::UnknownError);
        tWarn("Unable to remove the '%s' object. Create it before!", metaObject()->className());
        return false;
    }

    QSqlDatabase &database = Tf::currentSqlDatabase(databaseId());
    QString del = database.driver()->sqlStatement(QSqlDriver::DeleteStatement, tableName(), *static_cast<QSqlRecord *>(this), false);
    if (del.isEmpty()) {
        sqlError = QSqlError(QLatin1String("Unable to delete row"),
                             QString(), QSqlError::StatementError);
        return false;
    }

    del.append(" WHERE ");
    int revIndex = -1;

    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i) {
        const char *propName = metaObject()->property(i).name();
        QByteArray prop = QByteArray(propName).toLower();

        if (prop == LockRevision) {
            bool ok;
            int revision = property(propName).toInt(&ok);

            if (!ok || revision <= 0) {
                sqlError = QSqlError(QLatin1String("Unable to convert the 'revision' property to an int"),
                                     QString(), QSqlError::UnknownError);
                tError("Unable to convert the 'revision' property to an int, %s", qPrintable(objectName()));
                return false;
            }

            del.append(QLatin1String(propName));
            del.append("=").append(TSqlQuery::formatValue(revision, database));
            del.append(" AND ");

            revIndex = i;
            break;
        }
    }

    const char *pkName = metaObject()->property(metaObject()->propertyOffset() + primaryKeyIndex()).name();
    if (primaryKeyIndex() < 0 || !pkName) {
        QString msg = QString("Primary key not found for table ") + tableName() + QLatin1String(". Create a primary key!");
        sqlError = QSqlError(msg, QString(), QSqlError::StatementError);
        tError("%s", qPrintable(msg));
        return false;
    }
    del.append(QLatin1String(pkName));
    del.append("=").append(TSqlQuery::formatValue(value(pkName), database));

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
            tWarn("Row was deleted by another transaction, %s", qPrintable(tableName()));
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
    QSqlRecord::operator=(Tf::currentSqlDatabase(databaseId()).record(tableName()));
    const QMetaObject *metaObj = metaObject();
    for (int i = metaObj->propertyOffset(); i < metaObj->propertyCount(); ++i) {
        const char *propName = metaObj->property(i).name();
        int idx = indexOf(propName);
        if (idx >= 0) {
            QSqlRecord::setValue(idx, QObject::property(propName));
        } else {
            tWarn("invalid name: %s", propName);
        }
    }
}
