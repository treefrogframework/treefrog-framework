/* Copyright (c) 2011-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

/**
 * Class to paginate a list of items in a old digg style
 *
 * @author Vo Xuan Tien <tien.xuan.vo@gmail.com>
 * @author Darko Goleš
 * @author Carlos Mafla <gigo6000@hotmail.com>
 * @www.inchoo.net
 */

#include <TPaginator>
#include <QtCore>

/*!
  \class TPaginator
  \brief The TPaginator class provide simple solution to show paging toolbar.
*/

/*!
  Constructor.
*/
TPaginator::TPaginator(int itemsCount, int limit, int midRange)
    : currentPage_(1)
{
    itemsCount_ = qMax(itemsCount, 0);
    limit_ = qMax(limit, 1);

    // Change even number to larger odd number
    midRange = qMax(midRange, 1);
    midRange_ = midRange + (((midRange % 2) == 0) ? 1 : 0);

    calculateNumPages();
    calculateOffset();
    calculateRange();
}

/*!
  Calculate number of pages.
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
  Calculates offset for items based on current page number.
 */
void TPaginator::calculateOffset()
{
    offset_ = (currentPage_ - 1) * limit_;
}

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

void TPaginator::setItemsCount(int itemsCount)
{
    itemsCount_ = qMax(itemsCount, 0);

    // ItemsCount changes cause NumPages and Range change
    calculateNumPages();
    calculateRange();
}

void TPaginator::setLimit(int limit)
{
    limit_ = qMax(limit, 1);

    // Limit changes cause NumPages, Offset and Range change
    calculateNumPages();
    calculateOffset();
    calculateRange();
}

void TPaginator::setMidRange(int midRange)
{
    // Change even number to larger odd number
    midRange = qMax(midRange, 1);
    midRange_ = midRange + (((midRange % 2) == 0) ? 1 : 0);

    // MidRange changes cause Range changes
    calculateRange();
}

void TPaginator::setCurrentPage(int page)
{
    currentPage_ = isValidPage(page) ? page : 1;

    // CurrentPage changes cause Offset and Range change
    calculateOffset();
    calculateRange();
}
