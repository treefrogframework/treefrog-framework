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
  \brief The TPaginator class provide simple solution to show paging toolbar.
*/

/*!
  Constructor.
*/
TPaginator::TPaginator(int itemsCount, int limit, int midRange)
{ 
    this->itemsCount = itemsCount;
    this->limit = limit;
    this->midRange = midRange;
    this->currentPage = 1;

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
    if (itemsCount < 0)
    {
        itemsCount = 0;
    }
    
    if (limit < 1)
    {
        limit = 10;
    }
    
    if (midRange < 1)
    {
        midRange = 3;
    }
}

/*!
  Calculate number of pages.
  Internal use only.
*/
void TPaginator::calculateNumPages()
{
    //If limit is larger than or equal to total items count
    //display all in one page
    if (limit >= itemsCount)
    {
        numPages = 1;
    }
    else
    {
        //Calculate rest numbers from dividing operation so we can add one 
        //more page for this items
        int restItemsNum = itemsCount % limit;
        //if rest items > 0 then add one more page else just divide items 
        //by limit
        numPages = restItemsNum > 0 ? int(itemsCount / limit) + 1 : int(itemsCount / limit);
    }
}

/*!
  Calculate offset. 
 */
void TPaginator::calculateOffset()
{
    //Calculet offset for items based on current page number
    offset = (currentPage - 1) * limit;
}

void TPaginator::calculateRange()
{
    range = QList<int>();

    int startRange = currentPage - qFloor(midRange / 2);
    int endRange = currentPage + qFloor(midRange / 2);

    if (startRange < 1)
    {
        startRange = 1;
    }

    if (endRange > numPages)
    {
        endRange = numPages;
    }

    for (int i = startRange; i <= endRange; i++)
    {
        range << i;
    }
}

void TPaginator::setItemsCount(int itemsCount)
{
    if (itemsCount < 0)
    {
        itemsCount = 0;
    }

    this->itemsCount = itemsCount;

    this->calculateNumPages();
    this->calculateRange();
}

void TPaginator::setLimit(int limit)
{
    if (limit < 1)
    {
        limit = 10;
    }

    this->limit = limit;

    this->calculateNumPages();
    this->calculateOffset();
    this->calculateRange();
}

void TPaginator::setMidRange(int midRange)
{
    if (midRange < 1)
    {
        midRange = 3;
    }

    this->midRange = midRange;

    this->calculateRange();
}

void TPaginator::setCurrentPage(int page)
{
    if (isValidPage(page))
    {
        this->currentPage = page;
    }
    else
    {
        this->currentPage = 1;
    }

    this->calculateOffset();
    this->calculateRange();
}
