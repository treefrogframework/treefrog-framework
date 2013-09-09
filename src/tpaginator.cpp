/* Copyright (c) 2011-2013, AOYAMA Kazuharu
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

#include <TPaginator>
#include <QtCore>

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
TPaginator::TPaginator(int itemsTotal, int itemsPerPage , int midRange)
    : currentPage_(1)
{
    itemsTotal_ = qMax(itemsTotal, 0);
    itemsPerPage_ = qMax(itemsPerPage, 1);

    // midRange must be odd number
    midRange = qMax(midRange, 1);
    midRange_ = midRange + (((midRange % 2) == 0) ? 1 : 0);

    calculateNumPages();
}

/*!
  Calculates the number of total pages.
  Internal use only.
*/
void TPaginator::calculateNumPages()
{
    numPages_ = (itemsTotal_ / itemsPerPage_) + ((itemsTotal_ % itemsPerPage_) ? 1 : 0);
    numPages_ = qMax(numPages_, 1);

    // validation of currentPage
    if (currentPage_ > numPages_) {
        currentPage_ = 1;  // default value
    }
}

/*!
  Returns the number of items before the first item of the current
  page.
*/
int TPaginator::offset() const
{
    return (currentPage_ - 1) * itemsPerPage_;
}

void TPaginator::setItemsCount(int count)
{
    itemsTotal_ = qMax(count, 0);
    calculateNumPages();
}

/*!
  Sets the total number of items to \a total and recalculates other
  parameters.
*/
void TPaginator::setItemTotalCount(int total)
{
    itemsTotal_ = qMax(total, 0);
    calculateNumPages();
}

void TPaginator::setLimit(int limit)
{
    itemsPerPage_ = qMax(limit, 1);
    calculateNumPages();
}

/*!
  Sets the maximum number of items to be shown per page to \a count,
  and recalculates other parameters.
*/
void TPaginator::setItemCountPerPage(int count)
{
    itemsPerPage_ = qMax(count, 1);
    calculateNumPages();
}

/*!
  Sets the number of page numbers to \a range and recalculates other
  parameters.
*/
void TPaginator::setMidRange(int range)
{
    // Change even number to larger odd number
    range = qMax(range, 1);
    midRange_ = range + ((range % 2) ? 0 : 1);
}

/*!
  Sets the current page to \a page and recalculates other parameters.
*/
void TPaginator::setCurrentPage(int page)
{
    currentPage_ = isValidPage(page) ? page : 1;
}

/*!
  Returns a list of page numbers to be shown on a pagination bar.
*/
QList<int> TPaginator::range() const
{
    QList<int> ret;
    int start = qMax(currentPage_ - qFloor(midRange_ / 2), 1);
    int end = qMin(currentPage_ + qFloor(midRange_ / 2), numPages_);

    if (start == 1) {
        end = qMin(start + midRange_ - 1, numPages_);
    } else if (end == numPages_) {
        start = qMax(end - midRange_ + 1, 1);
    }

    for (int i = start; i <= end; ++i) {
        ret << i;
    }
    return ret;
}


/*!
  \fn int TPaginator::itemsCount() const
  Returns the number of items.
*/

/*!
  \fn int TPaginator::numPages() const
  Returns the total number of pages.
*/

/*!
  \fn int TPaginator::limit() const
  Returns the maximum number of items to be shown per page.
*/

/*!
  \fn int TPaginator::midRange() const
  Returns the number of page numbers to be shown on a pagination bar.
*/

/*!
  \fn int TPaginator::currentPage() const
  Returns the current page number.
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
  \fn bool TPaginator::hasPreviousPage() const
  Returns true if there is at least one page before the current page;
  otherwise returns false.
*/

/*!
  \fn bool TPaginator::hasNextPage() const
  Returns true if there is at least one page after the current page;
  otherwise returns false.
*/

/*!
  \fn bool TPaginator::isValidPage(int page) const
  Returns true if \a page is a valid page; otherwise returns false.
*/
