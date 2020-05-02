/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 *
 * Author: Darko Goleš
 * Author: Carlos Mafla <gigo6000@hotmail.com>
 * Author: Vo Xuan Tien <tien.xuan.vo@gmail.com>
 * Modified by AOYAMA Kazuharu
 */

#include <QtCore>
#include <TPaginator>
#include <cmath>

/*!
  \class TPaginator
  \brief The TPaginator class provides simple functionality for a pagination
  bar.
*/

/*!
  Constructs a TPaginator object using the parameters.
  \a itemsTotal specifies the total number of items.
  \a itemsPerPage specifies the maximum number of items to be shown per page.
  \a midRange specifies the number of pages to show ‘around’ the
  current page on a pagination bar, and should be an odd number.
*/
TPaginator::TPaginator(int itemsTotal, int itemsPerPage, int midRange) :
    _itemsTotal(itemsTotal),
    _itemsPerPage(itemsPerPage),
    _midRange(midRange)
{
    calculateNumPages();
}

/*!
  Copy constructor.
*/
TPaginator::TPaginator(const TPaginator &other) :
    _itemsTotal(other._itemsTotal),
    _itemsPerPage(other._itemsPerPage),
    _midRange(other._midRange),
    _numPages(other._numPages),
    _currentPage(other._currentPage)
{
}

/*!
  Assignment operator
*/
TPaginator &TPaginator::operator=(const TPaginator &other)
{
    _itemsTotal = other._itemsTotal;
    _itemsPerPage = other._itemsPerPage;
    _midRange = other._midRange;
    _numPages = other._numPages;
    _currentPage = other._currentPage;
    return *this;
}

/*!
  Calculates the total number of pages.
  Internal use only.
*/
void TPaginator::calculateNumPages()
{
    _itemsTotal = qMax(_itemsTotal, 0);
    _itemsPerPage = qMax(_itemsPerPage, 1);

    // midRange must be odd number
    _midRange = qMax(_midRange, 1);
    _midRange = (_midRange % 2) ? _midRange : _midRange + 1;

    // total number of pages
    _numPages = qMax((int)ceil(_itemsTotal / (double)_itemsPerPage), 1);
}

/*!
  Returns the number of items before the first item of the current
  page.
*/
int TPaginator::offset() const
{
    return (currentPage() - 1) * _itemsPerPage;
}

/*!
  Sets the total number of items to \a total and recalculates other
  parameters.
*/
void TPaginator::setItemTotalCount(int total)
{
    _itemsTotal = total;
    calculateNumPages();
}

/*!
  Sets the maximum number of items to be shown per page to \a count,
  and recalculates other parameters.
*/
void TPaginator::setItemCountPerPage(int count)
{
    _itemsPerPage = count;
    calculateNumPages();
}

/*!
  Sets the number of page numbers to \a range and recalculates other
  parameters.
*/
void TPaginator::setMidRange(int range)
{
    _midRange = range;
    calculateNumPages();
}

/*!
  Sets the current page to \a page and recalculates other parameters.
*/
void TPaginator::setCurrentPage(int page)
{
    _currentPage = page;
}

/*!
  Returns a list of page numbers to be shown on a pagination bar.
*/
QList<int> TPaginator::range() const
{
    QList<int> ret;

    int start = qMax(currentPage() - _midRange / 2, 1);
    int end;

    if (start == 1) {
        end = qMin(_midRange, _numPages);
    } else {
        end = qMin(currentPage() + _midRange / 2, _numPages);

        if (end == _numPages) {
            start = qMax(end - _midRange + 1, 1);
        }
    }

    for (int i = start; i <= end; ++i) {
        ret << i;
    }
    return ret;
}

/*!
  Returns the number of items of current page.
*/
int TPaginator::itemCountOfCurrentPage() const
{
    return qBound(0, _itemsTotal - offset(), _itemsPerPage);
}

/*!
  Returns the current page number.
*/
int TPaginator::currentPage() const
{
    return hasPage(_currentPage) ? _currentPage : 1;
}

/*!
  \fn int TPaginator::itemTotalCount() const
  Returns the total number of items.
*/

/*!
  \fn int TPaginator::numPages() const
  Returns the total number of pages.
*/

/*!
  \fn int TPaginator::itemCountPerPage() const
  Returns the maximum number of items to be shown per page.
*/

/*!
  \fn int TPaginator::midRange() const
  Returns the number of page numbers to be shown on a pagination bar.
*/

/*!
  \fn int TPaginator::firstPage() const
  Returns the first page number, always 1.
*/

/*!
  \fn int TPaginator::previousPage() const
  Returns the page number before the current page.
*/

/*!
  \fn int TPaginator::nextPage() const
  Returns the page number after the current page.
*/

/*!
  \fn int TPaginator::lastPage() const
  Returns the last page number.
*/

/*!
  \fn bool TPaginator::hasPrevious() const
  Returns true if there is at least one page before the current page;
  otherwise returns false.
*/

/*!
  \fn bool TPaginator::hasNext() const
  Returns true if there is at least one page after the current page;
  otherwise returns false.
*/

/*!
  \fn bool TPaginator::hasPage(int page) const
  Returns true if \a page is a valid page; otherwise returns false.
*/
