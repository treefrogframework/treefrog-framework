/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TSqlQueryORMapperIterator>

/*!
  \class TSqlQueryORMapperIterator
  \brief The TSqlQueryORMapperIterator class provides a Java-style
         iterator for TSqlQueryORMapper.
*/


/*!
  \fn TSqlQueryORMapperIterator<T>::TSqlQueryORMapperIterator(TSqlQueryORMapper<T> &)
  Constructs a TSqlQueryORMapperIterator object using the mapper \a mapper.
*/

/*!
  \fn bool TSqlQueryORMapperIterator<T>::hasNext() const
  Returns true if there is at least one object ahead of the iterator;
  otherwise returns false.
*/

/*!
  \fn bool TSqlQueryORMapperIterator<T>::hasPrevious() const
  Returns true if there is at least one object behind the iterator;
  otherwise returns false.
*/

/*!
  \fn void TSqlQueryORMapperIterator<T>::toBack()
  Moves the iterator to the back of the results (after the last
  object).
*/

/*!
  \fn void TSqlQueryORMapperIterator<T>::toFront()
  Moves the iterator to the front of the results (before the first
  object).
*/

/*!
  \fn T TSqlQueryORMapperIterator<T>::value() const
  Returns the current object and does not move the iterator.
*/
