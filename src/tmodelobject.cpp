/* Copyright (c) 2013-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QMetaProperty>
#include <TModelObject>

/*!
  \class TModelObject
  \brief The TModelObject class provides an abstract base for model objects
*/

/*!
  Returns a map object of the properties.
*/
QVariantMap TModelObject::toVariantMap() const
{
    QVariantMap ret;
    const QMetaObject *metaObj = metaObject();
    for (int i = metaObj->propertyOffset(); i < metaObj->propertyCount(); ++i) {
        const char *propName = metaObj->property(i).name();
        QString n(propName);
        if (!n.isEmpty()) {
            ret.insert(n, QObject::property(propName));
        }
    }
    return ret;
}

/*!
  Set the \a values to the properties of the object.
*/
void TModelObject::setProperties(const QVariantMap &values)
{
    const QMetaObject *metaObj = metaObject();
    for (int i = metaObj->propertyOffset(); i < metaObj->propertyCount(); ++i) {
        const char *n = metaObj->property(i).name();
        QLatin1String key(n);
        if (values.contains(key)) {
            QObject::setProperty(n, values[key]);
        }
    }
}

/*!
  Clears the contents of the object.
*/
void TModelObject::clear()
{
}

/*!
  Returns a list of the property names.
*/
QStringList TModelObject::propertyNames() const
{
    QStringList ret;
    const QMetaObject *metaObj = metaObject();
    for (int i = metaObj->propertyOffset(); i < metaObj->propertyCount(); ++i) {
        const char *propName = metaObj->property(i).name();
        ret << QLatin1String(propName);
    }
    return ret;
}
