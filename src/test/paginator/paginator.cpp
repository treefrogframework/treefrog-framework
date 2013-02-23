#include <TfTest/TfTest>
#include "../../tpaginator.h"


class TestPaginator : public QObject
{
    Q_OBJECT
private slots:
    void testConstructor();
    void testSetItemsCount();
    void testSetLimit();
    void testSetMidRange();
    void testSetCurrentPage();
    void testIsValidPage();
    void testItemsCountChangesMakeCurrentPageInvalid();
    void testLimitChangesMakeCurrentPageInvalid();
};


void TestPaginator::testConstructor()
{
    QList<int> range;

    // No args
    TPaginator pager;
    QCOMPARE(pager.getItemsCount(), 0);
    QCOMPARE(pager.getNumPages(), 1);
    QCOMPARE(pager.getLimit(), 1);
    QCOMPARE(pager.getOffset(), 0);
    QCOMPARE(pager.getMidRange(), 1);
    range = QList<int>();
    range.append(1);
    QCOMPARE(pager.getRange(), range);

    // ItemsCount < 0
    pager = TPaginator(-9, 4, 5);
    QCOMPARE(pager.getItemsCount(), 0);
    QCOMPARE(pager.getNumPages(), 1);
    QCOMPARE(pager.getLimit(), 4);
    QCOMPARE(pager.getOffset(), 0);
    QCOMPARE(pager.getMidRange(), 5);
    range = QList<int>();
    range.append(1);
    QCOMPARE(pager.getRange(), range);

    // ItemsCount = 0
    pager = TPaginator(0, 4, 5);
    QCOMPARE(pager.getItemsCount(), 0);
    QCOMPARE(pager.getNumPages(), 1);
    QCOMPARE(pager.getLimit(), 4);
    QCOMPARE(pager.getOffset(), 0);
    QCOMPARE(pager.getMidRange(), 5);
    range = QList<int>();
    range.append(1);
    QCOMPARE(pager.getRange(), range);

    // Limit < 1
    pager = TPaginator(158, -3, 5);
    QCOMPARE(pager.getItemsCount(), 158);
    QCOMPARE(pager.getNumPages(), 158);
    QCOMPARE(pager.getLimit(), 1);
    QCOMPARE(pager.getOffset(), 0);
    QCOMPARE(pager.getMidRange(), 5);
    range = QList<int>();
    range.append(1);
    range.append(2);
    range.append(3);
    QCOMPARE(pager.getRange(), range);

    // Limit = 1
    pager = TPaginator(158, 1, 5);
    QCOMPARE(pager.getItemsCount(), 158);
    QCOMPARE(pager.getNumPages(), 158);
    QCOMPARE(pager.getLimit(), 1);
    QCOMPARE(pager.getOffset(), 0);
    QCOMPARE(pager.getMidRange(), 5);
    range = QList<int>();
    range.append(1);
    range.append(2);
    range.append(3);
    QCOMPARE(pager.getRange(), range);

    // MidRange < 1
    pager = TPaginator(158, 4, -98);
    QCOMPARE(pager.getItemsCount(), 158);
    QCOMPARE(pager.getNumPages(), 40);
    QCOMPARE(pager.getLimit(), 4);
    QCOMPARE(pager.getOffset(), 0);
    QCOMPARE(pager.getMidRange(), 1);
    range = QList<int>();
    range.append(1);
    QCOMPARE(pager.getRange(), range);

    // MidRange = 1
    pager = TPaginator(158, 4, 1);
    QCOMPARE(pager.getItemsCount(), 158);
    QCOMPARE(pager.getNumPages(), 40);
    QCOMPARE(pager.getLimit(), 4);
    QCOMPARE(pager.getOffset(), 0);
    QCOMPARE(pager.getMidRange(), 1);
    range = QList<int>();
    range.append(1);
    QCOMPARE(pager.getRange(), range);

    // MidRange = even number
    pager = TPaginator(158, 4, 6);
    QCOMPARE(pager.getItemsCount(), 158);
    QCOMPARE(pager.getNumPages(), 40);
    QCOMPARE(pager.getLimit(), 4);
    QCOMPARE(pager.getOffset(), 0);
    QCOMPARE(pager.getMidRange(), 7);
    range = QList<int>();
    range.append(1);
    range.append(2);
    range.append(3);
    range.append(4);
    QCOMPARE(pager.getRange(), range);
}

void TestPaginator::testSetItemsCount()
{
    QList<int> range;
    TPaginator pager(158, 4, 5);

    // ItemsCount < 0
    pager.setItemsCount(-4);
    QCOMPARE(pager.getItemsCount(), 0);
    QCOMPARE(pager.getNumPages(), 1);
    QCOMPARE(pager.getLimit(), 4);
    QCOMPARE(pager.getOffset(), 0);
    QCOMPARE(pager.getMidRange(), 5);
    range = QList<int>();
    range.append(1);
    QCOMPARE(pager.getRange(), range);

    // ItemsCount = 0
    pager.setItemsCount(0);
    QCOMPARE(pager.getItemsCount(), 0);
    QCOMPARE(pager.getNumPages(), 1);
    QCOMPARE(pager.getLimit(), 4);
    QCOMPARE(pager.getOffset(), 0);
    QCOMPARE(pager.getMidRange(), 5);
    range = QList<int>();
    range.append(1);
    QCOMPARE(pager.getRange(), range);
}

void TestPaginator::testSetLimit()
{
    QList<int> range;
    TPaginator pager(158, 4, 5);

    // Limit < 1
    pager.setLimit(0);
    QCOMPARE(pager.getItemsCount(), 158);
    QCOMPARE(pager.getNumPages(), 158);
    QCOMPARE(pager.getLimit(), 1);
    QCOMPARE(pager.getOffset(), 0);
    QCOMPARE(pager.getMidRange(), 5);
    range = QList<int>();
    range.append(1);
    range.append(2);
    range.append(3);
    QCOMPARE(pager.getRange(), range);

    // Limit = 1
    pager.setLimit(1);
    QCOMPARE(pager.getItemsCount(), 158);
    QCOMPARE(pager.getNumPages(), 158);
    QCOMPARE(pager.getLimit(), 1);
    QCOMPARE(pager.getOffset(), 0);
    QCOMPARE(pager.getMidRange(), 5);
    range = QList<int>();
    range.append(1);
    range.append(2);
    range.append(3);
    QCOMPARE(pager.getRange(), range);
}

void TestPaginator::testSetMidRange()
{
    QList<int> range;
    TPaginator pager(158, 4, 5);
    pager.setCurrentPage(20);

    // MidRange < 1
    pager.setMidRange(-16);
    QCOMPARE(pager.getItemsCount(), 158);
    QCOMPARE(pager.getNumPages(), 40);
    QCOMPARE(pager.getLimit(), 4);
    QCOMPARE(pager.getOffset(), 76);
    QCOMPARE(pager.getMidRange(), 1);
    range = QList<int>();
    range.append(20);// Current page
    QCOMPARE(pager.getRange(), range);

    // MidRange = 1
    pager.setMidRange(1);
    QCOMPARE(pager.getItemsCount(), 158);
    QCOMPARE(pager.getNumPages(), 40);
    QCOMPARE(pager.getLimit(), 4);
    QCOMPARE(pager.getOffset(), 76);
    QCOMPARE(pager.getMidRange(), 1);
    range = QList<int>();
    range.append(20);// Current page
    QCOMPARE(pager.getRange(), range);

    // MidRange = even number
    pager.setMidRange(8);
    QCOMPARE(pager.getItemsCount(), 158);
    QCOMPARE(pager.getNumPages(), 40);
    QCOMPARE(pager.getLimit(), 4);
    QCOMPARE(pager.getOffset(), 76);
    QCOMPARE(pager.getMidRange(), 9);
    range = QList<int>();
    range.append(16);
    range.append(17);
    range.append(18);
    range.append(19);
    range.append(20);// Current page
    range.append(21);
    range.append(22);
    range.append(23);
    range.append(24);
    QCOMPARE(pager.getRange(), range);

    // MidRange = Nb.Pages - 1
    pager.setMidRange(39);
    QCOMPARE(pager.getItemsCount(), 158);
    QCOMPARE(pager.getNumPages(), 40);
    QCOMPARE(pager.getLimit(), 4);
    QCOMPARE(pager.getOffset(), 76);
    QCOMPARE(pager.getMidRange(), 39);
    range = QList<int>();
    range << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12 << 13 << 14 << 15 << 16 << 17 << 18 << 19;
    range << 20; // Current page
    range << 21 << 22 << 23 << 24 << 25 << 26 << 27 << 28 << 29 << 30 << 31 << 32 << 33 << 34 << 35 << 36 << 37 << 38 << 39;
    QCOMPARE(pager.getRange(), range);

    // MidRange = Nb.Pages && Nb.Pages = even number
    pager.setMidRange(40);
    QCOMPARE(pager.getItemsCount(), 158);
    QCOMPARE(pager.getNumPages(), 40);
    QCOMPARE(pager.getLimit(), 4);
    QCOMPARE(pager.getOffset(), 76);
    QCOMPARE(pager.getMidRange(), 41);
    range = QList<int>();
    range << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12 << 13 << 14 << 15 << 16 << 17 << 18 << 19;
    range << 20; // Current page
    range << 21 << 22 << 23 << 24 << 25 << 26 << 27 << 28 << 29 << 30 << 31 << 32 << 33 << 34 << 35 << 36 << 37 << 38 << 39 << 40;
    QCOMPARE(pager.getRange(), range);

    // MidRange > Nb.Pages
    pager.setMidRange(53);
    QCOMPARE(pager.getItemsCount(), 158);
    QCOMPARE(pager.getNumPages(), 40);
    QCOMPARE(pager.getLimit(), 4);
    QCOMPARE(pager.getOffset(), 76);
    QCOMPARE(pager.getMidRange(), 53);
    range = QList<int>();
    range << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12 << 13 << 14 << 15 << 16 << 17 << 18 << 19;
    range << 20; // Current page
    range << 21 << 22 << 23 << 24 << 25 << 26 << 27 << 28 << 29 << 30 << 31 << 32 << 33 << 34 << 35 << 36 << 37 << 38 << 39 << 40;
    QCOMPARE(pager.getRange(), range);

    pager = TPaginator(161, 4, 5);
    pager.setCurrentPage(21);

    // MidRange = Nb.Pages && Nb.Pages = odd number
    pager.setMidRange(41);
    QCOMPARE(pager.getItemsCount(), 161);
    QCOMPARE(pager.getNumPages(), 41);
    QCOMPARE(pager.getLimit(), 4);
    QCOMPARE(pager.getOffset(), 80);
    QCOMPARE(pager.getMidRange(), 41);
    range = QList<int>();
    range << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12 << 13 << 14 << 15 << 16 << 17 << 18 << 19 << 20;
    range << 21; // Current page
    range << 22 << 23 << 24 << 25 << 26 << 27 << 28 << 29 << 30 << 31 << 32 << 33 << 34 << 35 << 36 << 37 << 38 << 39 << 40 << 41;
    QCOMPARE(pager.getRange(), range);
}

void TestPaginator::testSetCurrentPage()
{
    QList<int> range;
    TPaginator pager(158, 4, 5);

    // Page < 1
    pager.setCurrentPage(-53);
    QCOMPARE(pager.getCurrentPage(), 1);
    QCOMPARE(pager.getFirstPage(), 1);
    QCOMPARE(pager.getPreviousPage(), 1);
    QCOMPARE(pager.getNextPage(), 2);
    QCOMPARE(pager.getLastPage(), 40);
    QCOMPARE(pager.haveToPaginate(), true);
    QCOMPARE(pager.isFirstPageEnabled(), false);
    QCOMPARE(pager.hasPreviousPage(), false);
    QCOMPARE(pager.hasNextPage(), true);
    QCOMPARE(pager.isLastPageEnabled(), true);
    range = QList<int>();
    range.append(1);// Current Page
    range.append(2);
    range.append(3);
    QCOMPARE(pager.getRange(), range);

    // Page = 1
    pager.setCurrentPage(1);
    QCOMPARE(pager.getCurrentPage(), 1);
    QCOMPARE(pager.getFirstPage(), 1);
    QCOMPARE(pager.getPreviousPage(), 1);
    QCOMPARE(pager.getNextPage(), 2);
    QCOMPARE(pager.getLastPage(), 40);
    QCOMPARE(pager.haveToPaginate(), true);
    QCOMPARE(pager.isFirstPageEnabled(), false);
    QCOMPARE(pager.hasPreviousPage(), false);
    QCOMPARE(pager.hasNextPage(), true);
    QCOMPARE(pager.isLastPageEnabled(), true);
    range = QList<int>();
    range.append(1);// Current Page
    range.append(2);
    range.append(3);
    QCOMPARE(pager.getRange(), range);

    // Page <= floor(MidRange / 2)
    pager.setCurrentPage(2);
    QCOMPARE(pager.getCurrentPage(), 2);
    QCOMPARE(pager.getFirstPage(), 1);
    QCOMPARE(pager.getPreviousPage(), 1);
    QCOMPARE(pager.getNextPage(), 3);
    QCOMPARE(pager.getLastPage(), 40);
    QCOMPARE(pager.haveToPaginate(), true);
    QCOMPARE(pager.isFirstPageEnabled(), true);
    QCOMPARE(pager.hasPreviousPage(), true);
    QCOMPARE(pager.hasNextPage(), true);
    QCOMPARE(pager.isLastPageEnabled(), true);
    range = QList<int>();
    range.append(1);
    range.append(2);// Current Page
    range.append(3);
    range.append(4);
    QCOMPARE(pager.getRange(), range);

    // Page > floor(MidRange / 2) && Page <= Nb.Pages - floor(MidRange / 2)
    pager.setCurrentPage(25);
    QCOMPARE(pager.getCurrentPage(), 25);
    QCOMPARE(pager.getFirstPage(), 1);
    QCOMPARE(pager.getPreviousPage(), 24);
    QCOMPARE(pager.getNextPage(), 26);
    QCOMPARE(pager.getLastPage(), 40);
    QCOMPARE(pager.haveToPaginate(), true);
    QCOMPARE(pager.isFirstPageEnabled(), true);
    QCOMPARE(pager.hasPreviousPage(), true);
    QCOMPARE(pager.hasNextPage(), true);
    QCOMPARE(pager.isLastPageEnabled(), true);
    range = QList<int>();
    range.append(23);
    range.append(24);
    range.append(25);// Current Page
    range.append(26);
    range.append(27);
    QCOMPARE(pager.getRange(), range);

    // Page > Nb.Pages - floor(MidRange / 2)
    pager.setCurrentPage(39);
    QCOMPARE(pager.getCurrentPage(), 39);
    QCOMPARE(pager.getFirstPage(), 1);
    QCOMPARE(pager.getPreviousPage(), 38);
    QCOMPARE(pager.getNextPage(), 40);
    QCOMPARE(pager.getLastPage(), 40);
    QCOMPARE(pager.haveToPaginate(), true);
    QCOMPARE(pager.isFirstPageEnabled(), true);
    QCOMPARE(pager.hasPreviousPage(), true);
    QCOMPARE(pager.hasNextPage(), true);
    QCOMPARE(pager.isLastPageEnabled(), true);
    range = QList<int>();
    range.append(37);
    range.append(38);
    range.append(39);// Current Page
    range.append(40);
    QCOMPARE(pager.getRange(), range);

    // Page = Nb.Pages
    pager.setCurrentPage(40);
    QCOMPARE(pager.getCurrentPage(), 40);
    QCOMPARE(pager.getFirstPage(), 1);
    QCOMPARE(pager.getPreviousPage(), 39);
    QCOMPARE(pager.getNextPage(), 40);
    QCOMPARE(pager.getLastPage(), 40);
    QCOMPARE(pager.haveToPaginate(), true);
    QCOMPARE(pager.isFirstPageEnabled(), true);
    QCOMPARE(pager.hasPreviousPage(), true);
    QCOMPARE(pager.hasNextPage(), false);
    QCOMPARE(pager.isLastPageEnabled(), false);
    range = QList<int>();
    range.append(38);
    range.append(39);
    range.append(40);// Current Page
    QCOMPARE(pager.getRange(), range);

    // Page > Nb.Pages
    pager.setCurrentPage(59);
    QCOMPARE(pager.getCurrentPage(), 1);
    QCOMPARE(pager.getFirstPage(), 1);
    QCOMPARE(pager.getPreviousPage(), 1);
    QCOMPARE(pager.getNextPage(), 2);
    QCOMPARE(pager.getLastPage(), 40);
    QCOMPARE(pager.haveToPaginate(), true);
    QCOMPARE(pager.isFirstPageEnabled(), false);
    QCOMPARE(pager.hasPreviousPage(), false);
    QCOMPARE(pager.hasNextPage(), true);
    QCOMPARE(pager.isLastPageEnabled(), true);
    range = QList<int>();
    range.append(1);// Current Page
    range.append(2);
    range.append(3);
    QCOMPARE(pager.getRange(), range);
}

void TestPaginator::testIsValidPage()
{
    TPaginator pager(158, 4, 5);
    QCOMPARE(pager.isValidPage(-2), false);
    QCOMPARE(pager.isValidPage(1), true);
    QCOMPARE(pager.isValidPage(36), true);
    QCOMPARE(pager.isValidPage(40), true);
    QCOMPARE(pager.isValidPage(68), false);
}

void TestPaginator::testItemsCountChangesMakeCurrentPageInvalid()
{
    TPaginator pager(158, 4, 5);
    pager.setCurrentPage(21);
    pager.setItemsCount(16);
    QCOMPARE(pager.getCurrentPage(), 1);
}

void TestPaginator::testLimitChangesMakeCurrentPageInvalid()
{
    TPaginator pager(158, 4, 5);
    pager.setCurrentPage(21);
    pager.setLimit(50);
    QCOMPARE(pager.getCurrentPage(), 1);
}

//TF_TEST_SQLLESS_MAIN(TestPaginator)
TF_TEST_MAIN(TestPaginator)
#include "paginator.moc"
