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

static QString propertyNameToFieldName(const QString &name)
{
    QString field;
    for (int i = 0; i < name.length(); ++i) {
        if (name[i].isUpper()) {
            if (i > 0) {
                field += '_';
            }
            field += name[i].toLower();
        } else {
            field += name[i];
        }
    }
    return field;
}


static QString fieldNameToVariableName(const QString &name)
{
    QString var;
    for (int i = 0; i < name.length(); ++i) {
        if (name[i] != '_') {
            if (i > 0 && name[i - 1] == '_') {
                var += name[i].toUpper();
            } else {
                var += name[i].toLower();
            }
        }
    }
    return var;
}

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
  Returns a hash with all properties of this text format.
 */
QVariantHash TAbstractModel::properties() const
{
    QVariantHash res;
    for (QHashIterator<QString, QVariant> i(data()->properties()); i.hasNext(); ) {
        i.next();
        res.insert(fieldNameToVariableName(i.key()), i.value());
    }
    return res;
}

/*!
  Sets the \a properties.
 */
void TAbstractModel::setProperties(const QVariantHash &properties)
{
    QVariantHash prop;
    for (QHashIterator<QString, QVariant> i(properties); i.hasNext(); ) {
        i.next();
        prop.insert(propertyNameToFieldName(i.key()), i.value());
    }
    data()->setProperties(prop);
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
