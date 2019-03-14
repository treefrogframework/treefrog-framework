/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TSqlQueryORMapper>

/*!
  \class TSqlQueryORMapper
  \brief The TSqlQueryORMapper class is a template class that creates
         ORM objects by executing and manipulating SQL statements.
*/


/*!
  \fn TSqlQueryORMapper<T>::TSqlQueryORMapper(const QString &, int)
  Constructs a TSqlQueryORMapper object using the SQL \a query and the
  database \a databaseId. The \a query string must be to retrieve the
  ORM objects specified by the class \a T.
*/


/*!
  \fn TSqlQueryORMapper<T>::TSqlQueryORMapper(int)
  Constructs a TSqlQueryORMapper object using the database \a databaseId.
*/


/*!
  \fn TSqlQueryORMapper<T> &TSqlQueryORMapper<T>::prepare(const QString &)
  Prepares the SQL query \a query to retrieve the ORM objects specified by
  the class \a T.
*/


/*!
  \fn int TSqlQueryORMapper<T>::find()
  Executes the prepared SQL query and returns the number of the ORM objects.
  TSqlQueryORMapperIterator is used to get the retrieved ORM objects.
*/


/*!
  \fn T TSqlQueryORMapper<T>::findFirst()
  Executes the prepared SQL query and returns the first ORM object.
*/


/*!
  \fn T TSqlQueryORMapper<T>::value() const
  Returns the current ORM object in the results retrieved by the query.
*/


/*!
  \fn QString TSqlQueryORMapper<T>::fieldName(int index) const
  Returns the name of the field at position \a index in the class \a T.
  If the field does not exist, an empty string is returned.
*/
