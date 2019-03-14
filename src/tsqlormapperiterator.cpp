/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TSqlORMapperIterator>

/*!
  \class TSqlORMapperIterator
  \brief The TSqlORMapperIterator class provides a Java-style iterator
         for TSqlORMapper.
*/

/*!
  \fn TSqlORMapperIterator<T>::TSqlORMapperIterator(const TSqlORMapper<T> &)
  Constructor.
*/

/*!
  \fn bool TSqlORMapperIterator<T>::hasNext() const
  Returns true if there is at least one object ahead of the iterator;
  otherwise returns false.
*/

/*!
  \fn bool TSqlORMapperIterator<T>::hasPrevious() const
  Returns true if there is at least one object behind the iterator;
  otherwise returns false.
*/

/*!
  \fn T TSqlORMapperIterator<T>::next()
  Returns the next object and advances the iterator by one position.
*/

/*!
  \fn T TSqlORMapperIterator<T>::previous()
  Returns the previous object and moves the iterator back by one
  position.
*/

/*!
  \fn void TSqlORMapperIterator<T>::toBack()
  Moves the iterator to the back of the results (after the last
  object).
*/

/*!
  \fn void TSqlORMapperIterator<T>::toFront()
  Moves the iterator to the front of the results (before the first
  object).
*/

/*!
  \fn T TSqlORMapperIterator<T>::value() const
  Returns the current object and does not move the iterator.
*/
