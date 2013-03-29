/* Copyright (c) 2011-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TPaginator>
#include <QtCore>

/*!
  \class TPaginator
  \brief The TPaginator class provides simple solution to show paging toolbar.
  This class can't be used to page data, but you can use offset and
  limit in a query statement to page data.
  \author Darko Goleš
  \author Carlos Mafla <gigo6000@hotmail.com>
  \author Vo Xuan Tien <tien.xuan.vo@gmail.com>
*/

/*!
  Constructor.
*/
TPaginator::TPaginator(int itemsCount, int limit, int midRange)
    : currentPage_(1)
{
    itemsCount_ = qMax(itemsCount, 0);
    limit_ = qMax(limit, 1);

    // Change even number to larger odd number.
    // midRange must be odd number.
    midRange = qMax(midRange, 1);
    midRange_ = midRange + (((midRange % 2) == 0) ? 1 : 0);

    calculateNumPages();
    calculateOffset();
    calculateRange();
}

/*!
  Calculates number of pages.
  Internal use only.
*/
void TPaginator::calculateNumPages()
{
    //If limit is larger than or equal to total items count
    //display all in one page
    if (limit_ >= itemsCount_) {
        numPages_ = 1;
    } else {
        //Calculate rest numbers from dividing operation so we can add one
        //more page for this items
        int restItemsNum = itemsCount_ % limit_;
        //if rest items > 0 then add one more page else just divide items
        //by limit
        numPages_ = (restItemsNum > 0) ? (itemsCount_ / limit_) + 1 : (itemsCount_ / limit_);
    }

    // If currentPage invalid after numPages changes
    if (currentPage_ > numPages_) {
        // Restore currentPage to default value
        currentPage_ = 1;
    }
}

/*!
  Calculates offset (start index of items to get from database).
  Internal use only.
*/
void TPaginator::calculateOffset()
{
    offset_ = (currentPage_ - 1) * limit_;
}

/*!
  Calculates range (pages will be show in paging toolbar).
  Internal use only.
*/
void TPaginator::calculateRange()
{
    int startRange = currentPage_ - qFloor(midRange_ / 2);
    int endRange = currentPage_ + qFloor(midRange_ / 2);

    // If invalid start range
    startRange = qMax(startRange, 1);

    // If invalid end range
    endRange = qMin(endRange, numPages_);

    range_.clear();
    for (int i = startRange; i <= endRange; i++) {
        range_ << i;
    }
}

/*!
  Sets number of items.
  Notice: changing \a itemsCount maybe make number of pages and range
  change also.
*/
void TPaginator::setItemsCount(int itemsCount)
{
    itemsCount_ = qMax(itemsCount, 0);

    // ItemsCount changes cause NumPages and Range change
    calculateNumPages();
    calculateRange();
}

/*!
  Sets limit.
  Notice: changing \a limit maybe make number of pages, offset and range
  change also.
*/
void TPaginator::setLimit(int limit)
{
    limit_ = qMax(limit, 1);

    // Limit changes cause NumPages, Offset and Range change
    calculateNumPages();
    calculateOffset();
    calculateRange();
}

/*!
  Sets mid range (number of pages will be show in paging toolbar).
  Notice: changing \a midRange maybe make range change also.
*/
void TPaginator::setMidRange(int midRange)
{
    // Change even number to larger odd number
    midRange = qMax(midRange, 1);
    midRange_ = midRange + (((midRange % 2) == 0) ? 1 : 0);

    // MidRange changes cause Range changes
    calculateRange();
}

/*!
  Sets current page.
  Notice: changing \a currentPage maybe make offset range change also.
*/
void TPaginator::setCurrentPage(int page)
{
    currentPage_ = isValidPage(page) ? page : 1;

    // CurrentPage changes cause Offset and Range change
    calculateOffset();
    calculateRange();
}

/*!
  \fn int TPaginator::itemsCount() const
  Gets number of items.
*/

/*!
  \fn int TPaginator::numPages() const
  Gets number of pages.
*/

/*!
  \fn int TPaginator::limit() const
  Gets limit (number items per page).
*/

/*!
  \fn int TPaginator::offset() const
  Gets offset (start index of items to get from database).
*/

/*!
  \fn int TPaginator::midRange() const
  Gets mid range (number of pages will be show in paging toolbar).
*/

/*!
  \fn int TPaginator::midRange() const
  Gets mid range (number of pages will be show in paging toolbar).
*/

/*!
  \fn const QList<int> &range() const
  Gets range (pages will be show in paging toolbar).
*/

/*!
  \fn int TPaginator::currentPage() const
  Gets current page that items will be show.
*/

/*!
  \fn int TPaginator::firstPage() const
  Gets first page.
*/

/*!
  \fn int TPaginator::previousPage() const
  Gets the page before current page.
*/

/*!
  \fn int TPaginator::nextPage() const
  Gets the page after current page.
*/

/*!
  \fn int TPaginator::lastPage() const
  Gets last page.
*/

/*!
  \fn bool TPaginator::haveToPaginate() const
  Whether paging toolbar will be display or not.
*/

/*!
  \fn bool TPaginator::isFirstPageEnabled() const
  Whether first page available or not.
*/

/*!
  \fn bool TPaginator::hasPreviousPage() const
  Whether previous page available or not.
*/

/*!
  \fn bool TPaginator::hasNextPage() const
  Whether next page available or not.
*/

/*!
  \fn bool TPaginator::isLastPageEnabled() const
  Whether last page available or not.
*/

/*!
  \fn bool TPaginator::isValidPage(int page) const
  Checks if \a page is a valid page or not.
*/
