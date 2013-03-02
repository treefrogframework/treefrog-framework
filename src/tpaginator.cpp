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

    // Change even number to larger odd number
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
  Calculate offset. 
 */
void TPaginator::calculateOffset()
{
    //Calculet offset for items based on current page number
    offset_ = (currentPage_ - 1) * limit_;
}

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
