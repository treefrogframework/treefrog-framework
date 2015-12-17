/* Copyright (c) 2013-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDateTime>
#include <QRegExp>
#include <QMetaProperty>
#include <TMongoObject>
#include <TMongoQuery>
#include <TAbstractModel>

const QByteArray LockRevision("lockRevision");
const QByteArray CreatedAt("createdAt");
const QByteArray UpdatedAt("updatedAt");
const QByteArray ModifiedAt("modifiedAt");


/*!
  Constructor.
*/
TMongoObject::TMongoObject()
    : TModelObject(), QVariantMap()
{ }

/*!
  Copy constructor.
*/
TMongoObject::TMongoObject(const TMongoObject &other)
    : TModelObject(), QVariantMap(other)
{ }

/*!
  Assignment operator.
*/
TMongoObject &TMongoObject::operator=(const TMongoObject &other)
{
    QVariantMap::operator=(*static_cast<const QVariantMap *>(&other));
    return *this;
}

/*!
  Returns the collection name, which is generated from the class name.
*/
QString TMongoObject::collectionName() const
{
    QString collection;
    QString clsname(metaObject()->className());

    for (int i = 0; i < clsname.length(); ++i) {
        if (i > 0 && clsname[i].isUpper()) {
            collection += '_';
        }
        collection += clsname[i].toLower();
    }
    collection.remove(QRegExp("_object$"));
    return collection;
}


void TMongoObject::setBsonData(const QVariantMap &bson)
{
    QVariantMap::operator=(bson);
    syncToObject();
}


bool TMongoObject::create()
{
    // Sets the values of 'created_at', 'updated_at' or 'modified_at' properties
    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i) {
        const char *propName = metaObject()->property(i).name();
        QString prop = TAbstractModel::fieldNameToVariableName(QString::fromLatin1(propName));

        if (prop == CreatedAt || prop == UpdatedAt || prop == ModifiedAt) {
            setProperty(propName, QDateTime::currentDateTime());
        } else if (prop == LockRevision) {
            // Sets the default value of 'revision' property
            setProperty(propName, 1);  // 1 : default value
        } else {
            // do nothing
        }
    }

    syncToVariantMap();
    QVariantMap::remove("_id"); // remove _id to generate internally

    TMongoQuery mongo(collectionName());
    bool ret = mongo.insert(*this);
    if (ret) {
        syncToObject();  // '_id' reflected
    }
    return ret;
}


bool TMongoObject::update()
{
    if (isNull()) {
        return false;
    }

    QVariantMap cri;

    // Updates the value of 'updated_at' or 'modified_at' property
    bool updflag = false;
    int revIndex = -1;

    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i) {
        const char *propName = metaObject()->property(i).name();
        QString prop = TAbstractModel::fieldNameToVariableName(QString::fromLatin1(propName));
QVariant V = property(propName);
        if (!updflag && (prop == UpdatedAt || prop == ModifiedAt)) {
            setProperty(propName, QDateTime::currentDateTime());
            updflag = true;

        } else if (revIndex < 0 && prop == LockRevision) {
            bool ok;
            int oldRevision = property(propName).toInt(&ok);

            if (!ok || oldRevision <= 0) {
                tError("Unable to convert the 'revision' property to an int, %s", qPrintable(objectName()));
                return false;
            }

            setProperty(propName, oldRevision + 1);
            revIndex = i;

            // add criteria
            cri[propName] = oldRevision;
        } else {
            // continue
        }
    }

    cri["_id"] = objectId();

    syncToVariantMap();
    TMongoQuery mongo(collectionName());
    bool ret = mongo.update(cri, *this);

    // Optimistic lock check
    if (revIndex >= 0 && mongo.numDocsAffected() != 1) {
        QString msg = QString("Doc was updated or deleted from table ") + collectionName();
        throw KvsException(msg, __FILE__, __LINE__);
    }

    return ret;
}


bool TMongoObject::remove()
{
    if (isNull()) {
        return false;
    }

    int revIndex = -1;
    QVariantMap cri;

    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i) {
        const char *propName = metaObject()->property(i).name();
        QString prop = TAbstractModel::fieldNameToVariableName(QString::fromLatin1(propName));

        if (prop == LockRevision) {
            bool ok;
            int revision = property(propName).toInt(&ok);

            if (!ok || revision <= 0) {
                tError("Unable to convert the 'revision' property to an int, %s", qPrintable(objectName()));
                return false;
            }

            revIndex = i;

            // add criteria
            cri[propName] = revision;
            break;
        }
    }

    cri.insert("_id", objectId());

    TMongoQuery mongo(collectionName());
    bool ret = mongo.remove(cri);
    QVariantMap::clear();

    // Optimistic lock check
    if (mongo.numDocsAffected() != 1) {
        if (revIndex >= 0) {
            QString msg = QString("Doc was updated or deleted from collection ") + collectionName();
            throw KvsException(msg, __FILE__, __LINE__);
        }
        tWarn("Doc was deleted by another transaction, %s", qPrintable(collectionName()));
    }

    return ret;
}


bool TMongoObject::reload()
{
    if (isNull()) {
        return false;
    }

    syncToObject();
    return true;
}


bool TMongoObject::isModified() const
{
    if (isNew())
        return false;

    int offset = metaObject()->propertyOffset();

    for (QMapIterator<QString, QVariant> it(*this); it.hasNext(); ) {
        it.next();
        QByteArray name = it.key().toLatin1();
        int index = metaObject()->indexOfProperty(name.constData());
        if (index >= offset) {
            if (it.value() != property(name.constData())) {
                return true;
            }
        }
    }
    return false;
}


void TMongoObject::syncToObject()
{
    int offset = metaObject()->propertyOffset();

    for (QMapIterator<QString, QVariant> it(*this); it.hasNext(); ) {
        it.next();
        QByteArray name = it.key().toLatin1();
        int index = metaObject()->indexOfProperty(name.constData());
        if (index >= offset) {
            QObject::setProperty(name.constData(), it.value());
        }
    }
}


void TMongoObject::syncToVariantMap()
{
    QVariantMap::clear();

    const QMetaObject *metaObj = metaObject();
    for (int i = metaObj->propertyOffset(); i < metaObj->propertyCount(); ++i) {
        const char *propName = metaObj->property(i).name();
        QVariantMap::insert(QLatin1String(propName), QObject::property(propName));
    }
}


void TMongoObject::clear()
{
    QVariantMap::clear();
    objectId().clear();
}
