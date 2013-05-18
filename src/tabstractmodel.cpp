/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TAbstractModel>
#include <TSqlObject>

/*!
  \class TAbstractModel
  \brief The TAbstractModel class is the abstract base class of models,
  providing functionality common to models.
*/

/*!
  Returns true if this model is null; otherwise returns false.
 */
bool TAbstractModel::isNull() const
{
    return data()->isNull();
}

/*!
  Returns true if this model is new, not created; otherwise returns false.
 */
bool TAbstractModel::isNew() const
{
    return data()->isNew();
}

/*!
  Returns true if this model is not saved; otherwise returns false.
 */
bool TAbstractModel::isSaved() const
{
    return !data()->isNew();
}

/*!
  Creates the model as a new data to the data storage.
 */
bool TAbstractModel::create()
{
    return data()->create();
}

/*!
  Saves the model to the data storage.

  If the model exists in the data storage, calls update();
  otherwise calls create().
 */
bool TAbstractModel::save()
{
    return (data()->isNull()) ? data()->create() : data()->update();
}

/*!
  Updates the model to the data storage.
 */
bool TAbstractModel::update()
{
    return data()->update();
}

/*!
  Removes the model from the data storage.
 */
bool TAbstractModel::remove()
{
    return data()->remove();
}

/*!
  Returns a map with all properties of this text format.
  Obsolete function.
*/
QVariantMap TAbstractModel::properties() const
{
    return toVariantMap();
}

/*!
  Returns a map with all properties of this text format.
 */
QVariantMap TAbstractModel::toVariantMap() const
{
    QVariantMap ret;

    QVariantMap map = data()->toVariantMap();
    for (QMapIterator<QString, QVariant> i(map); i.hasNext(); ) {
        i.next();
        ret.insert(fieldNameToVariableName(i.key()), i.value());
    }
    return ret;
}

/*!
  Sets the \a properties.
 */
void TAbstractModel::setProperties(const QVariantMap &properties)
{
    // Creates a map of the original property name and the converted name
    QStringList soprops = data()->propertyNames();
    QMap<QString, QString> sopropMap;
    for (QStringListIterator it(soprops); it.hasNext(); ) {
        const QString &orig = it.next();
        sopropMap.insert(fieldNameToVariableName(orig), orig);
    }

    QVariantMap props;
    for (QMapIterator<QString, QVariant> it(properties); it.hasNext(); ) {
        it.next();
        const QString &p = sopropMap[it.key()];
        if (!p.isEmpty()) {
            props.insert(p, it.value());
        }
    }

    data()->setProperties(props);
}


/*!
  \fn virtual TSqlObject *TAbstractModel::data()

  This function is reimplemented in subclasses to return a pointer
  to the data stored in the model object.
*/


/*!
  \fn virtual const TSqlObject *TAbstractModel::data() const

  This function is reimplemented in subclasses to return a pointer
  to the data stored in the model object.
*/
