/* Copyright (c) 2011-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

/**
 * Class to paginate a list of items in a old digg style
 *
 * @author Darko Goleš
 * @author Carlos Mafla <gigo6000@hotmail.com>
 * @author Vo Xuan Tien <tien.xuan.vo@gmail.com>
 * @www.inchoo.net
 */

#include <TPaginator>
#include <QtCore>

/*!
  \class TPaginator
  \brief The TPaginator class provide simple solution to show paging toolbar.
  Notice: this class can't be used to page data, but you can use offset and limit in a query statement to page data.
*/

/*!
  Constructor.
*/
TPaginator::TPaginator(int itemsCount, int limit, int midRange)
{
    this->itemsCount_ = itemsCount;
    this->limit_ = limit;
    this->midRange_ = midRange;
    this->currentPage_ = 1;

    this->setDefaults();

    this->calculateNumPages();
    this->calculateOffset();
    this->calculateRange();
}

/*!
  Change internal variables to default value.
  Internal use only.
*/
void TPaginator::setDefaults()
{
    if (itemsCount_ < 0)
    {
        // Default value
        itemsCount_ = 0;
    }
    
    if (limit_ < 1)
    {
        // Default value
        limit_ = 1;
    }
    
    if (midRange_ < 1)
    {
        // Default value
        midRange_ = 1;
    }

    // Change even number to larger odd number.
    // midRange must be odd number.
    midRange_ = midRange_ + (((midRange_ % 2) == 0) ? 1 : 0);
}

/*!
  Calculate number of pages.
  Internal use only.
*/
void TPaginator::calculateNumPages()
{
    //If limit is larger than or equal to total items count
    //display all in one page
    if (limit_ >= itemsCount_)
    {
        numPages_ = 1;
    }
    else
    {
        //Calculate rest numbers from dividing operation so we can add one 
        //more page for this items
        int restItemsNum = itemsCount_ % limit_;
        //if rest items > 0 then add one more page else just divide items 
        //by limit
        numPages_ = restItemsNum > 0 ? int(itemsCount_ / limit_) + 1 : int(itemsCount_ / limit_);
    }
    
    // If currentPage invalid after numPages changes
    if (currentPage_ > numPages_)
    {
        // Restore currentPage to default value
        currentPage_ = 1;
    }
}

/*!
  Calculate offset (start index of items to get from database).
  Internal use only.
 */
void TPaginator::calculateOffset()
{
    //Calculet offset for items based on current page number
    offset_ = (currentPage_ - 1) * limit_;
}

/*!
  Calculate range (pages will be show in paging toolbar).
  Internal use only.
 */
void TPaginator::calculateRange()
{
    range_ = QList<int>();

    int startRange = currentPage_ - qFloor(midRange_ / 2);
    int endRange = currentPage_ + qFloor(midRange_ / 2);

    // If invalid start range
    if (startRange < 1)
    {
        // Set start range = lowest value
        startRange = 1;
    }

    // If invalid end range
    if (endRange > numPages_)
    {
        // Set start range = largest value
        endRange = numPages_;
    }

    for (int i = startRange; i <= endRange; i++)
    {
        range_ << i;
    }
}

/*!
  Set number of items. Notice: changing \a itemsCount maybe make number of pages and range change also.
 */
void TPaginator::setItemsCount(int itemsCount)
{
    if (itemsCount < 0)
    {
        // Default value
        itemsCount = 0;
    }

    this->itemsCount_ = itemsCount;

    // ItemsCount changes cause NumPages and Range change
    this->calculateNumPages();
    this->calculateRange();
}

/*!
  Set limit. Notice: changing \a limit maybe make number of pages, offset and range change also.
 */
void TPaginator::setLimit(int limit)
{
    if (limit < 1)
    {
        // Default value
        limit = 1;
    }

    this->limit_ = limit;

    // Limit changes cause NumPages, Offset and Range change
    this->calculateNumPages();
    this->calculateOffset();
    this->calculateRange();
}

/*!
  Set mid range (number of pages will be show in paging toolbar). Notice: changing \a midRange maybe make range change also.
 */
void TPaginator::setMidRange(int midRange)
{
    if (midRange < 1)
    {
        // Default value
        midRange = 1;
    }

    // Change even number to larger odd number
    this->midRange_ = midRange + (((midRange % 2) == 0) ? 1 : 0);

    // MidRange changes cause Range changes
    this->calculateRange();
}

/*!
  Set current page. Notice: changing \a currentPage maybe make offset range change also.
 */
void TPaginator::setCurrentPage(int page)
{
    if (isValidPage(page))
    {
        this->currentPage_ = page;
    }
    else
    {
        // Default value
        this->currentPage_ = 1;
    }

    // CurrentPage changes cause Offset and Range change
    this->calculateOffset();
    this->calculateRange();
}

/*!
  \fn int TPaginator::itemsCount() const
  Get number of items.
*/

/*!
  \fn int TPaginator::numPages() const
  Get number of pages.
*/

/*!
  \fn int TPaginator::limit() const
  Get limit (number items per page).
*/

/*!
  \fn int TPaginator::offset() const
  Get offset (start index of items to get from database).
*/

/*!
  \fn int TPaginator::midRange() const
  Get mid range (number of pages will be show in paging toolbar).
*/

/*!
  \fn int TPaginator::midRange() const
  Get mid range (number of pages will be show in paging toolbar).
*/

/*!
  \fn const QList<int> &range() const
  Get range (pages will be show in paging toolbar).
*/

/*!
  \fn int TPaginator::currentPage() const
  Get current page that items will be show.
*/

/*!
  \fn int TPaginator::firstPage() const
  Get first page.
*/

/*!
  \fn int TPaginator::previousPage() const
  Get the page before current page.
*/

/*!
  \fn int TPaginator::nextPage() const
  Get the page after current page.
*/

/*!
  \fn int TPaginator::lastPage() const
  Get last page.
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
  Check if \a page is a valid page or not.
*/
