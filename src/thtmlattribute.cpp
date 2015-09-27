/* Copyright (c) 2010-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <THtmlAttribute>
#include <TActionView>

/*!
  \class THtmlAttribute
  \brief The THtmlAttribute class represents HTML attributes for customizing
  web elements.
*/

/*!
  Copy constructor.
*/
THtmlAttribute::THtmlAttribute(const THtmlAttribute &other)
    : QList<QPair<QString, QString>>(*static_cast<const QList<QPair<QString, QString>> *>(&other))
{ }

/*!
  Copy constructor.
*/
THtmlAttribute::THtmlAttribute(const QList<QPair<QString, QString>> &list)
    : QList<QPair<QString, QString>>(list)
{ }

/*!
  Returns true if the HTML attributes contains an item with key \a key;
  otherwise returns false.
*/
bool THtmlAttribute::contains(const QString &key) const
{
    for (QListIterator<QPair<QString, QString>> i(*this); i.hasNext(); ) {
        if (i.next().first == key)
            return true;
    }
    return false;
}

/*!
  Inserts a new item at the beginning of the attributes.
*/
void THtmlAttribute::prepend(const QString &key, const QString &value)
{
    QList<QPair<QString, QString>>::prepend(qMakePair(key, value));
}

/*!
  Inserts a new item at the end of the attributes.
*/
void THtmlAttribute::append(const QString &key, const QString &value)
{
    QList<QPair<QString, QString>>::append(qMakePair(key, value));
}

/*!
  Assignment operator.
*/
THtmlAttribute &THtmlAttribute::operator=(const THtmlAttribute &other)
{
    QList<QPair<QString, QString>>::operator=(*static_cast<const QList<QPair<QString, QString>> *>(&other));
    return *this;
}

/*!
  Returns HTML attributes that contains all the items in this attributes
  followed by all the items in the \a other attributes.
*/
THtmlAttribute THtmlAttribute::operator|(const THtmlAttribute &other) const
{
    THtmlAttribute attr(*this);
    return attr + other;
}

/*!
  Returns the attributes as a string. If \a escape is true, returns a escaped
  string.
*/
QString THtmlAttribute::toString(bool escape) const
{
    QString string;
    for (QListIterator<QPair<QString, QString>> i(*this); i.hasNext(); ) {
        const QPair<QString, QString> &p = i.next();
        string.append(" ").append(p.first);
        if (!p.second.isNull()) {
            string.append("=\"");
            string.append(escape ? THttpUtility::htmlEscape(p.second) : p.second);
            string.append("\"");
        }
    }
    return string;
}

/*!
  \fn THtmlAttribute::THtmlAttribute()
  Constructor.
*/
