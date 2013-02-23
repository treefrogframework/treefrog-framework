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
};


void TestPaginator::testConstructor()
{
    QList<int> range;

    // No args
    TPaginator pager;
    QCOMPARE(0, pager.getItemsCount());
    QCOMPARE(1, pager.getNumPages());
    QCOMPARE(10, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(3, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    QCOMPARE(range, pager.getRange());

    // ItemsCount < 0
    pager = TPaginator(-9, 4, 5);
    QCOMPARE(0, pager.getItemsCount());
    QCOMPARE(1, pager.getNumPages());
    QCOMPARE(4, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(5, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    QCOMPARE(range, pager.getRange());

    // ItemsCount = 0
    pager = TPaginator(0, 4, 5);
    QCOMPARE(0, pager.getItemsCount());
    QCOMPARE(1, pager.getNumPages());
    QCOMPARE(4, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(5, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    QCOMPARE(range, pager.getRange());

    // Limit < 1
    pager = TPaginator(158, -3, 5);
    QCOMPARE(158, pager.getItemsCount());
    QCOMPARE(16, pager.getNumPages());
    QCOMPARE(10, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(5, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    range.append(2);
    range.append(3);
    QCOMPARE(range, pager.getRange());

    // Limit = 1
    pager = TPaginator(158, 1, 5);
    QCOMPARE(158, pager.getItemsCount());
    QCOMPARE(158, pager.getNumPages());
    QCOMPARE(1, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(5, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    range.append(2);
    range.append(3);
    QCOMPARE(range, pager.getRange());

    // MidRange < 1
    pager = TPaginator(158, 4, -98);
    QCOMPARE(158, pager.getItemsCount());
    QCOMPARE(40, pager.getNumPages());
    QCOMPARE(4, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(3, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    range.append(2);
    QCOMPARE(range, pager.getRange());

    // MidRange = 1
    pager = TPaginator(158, 4, 1);
    QCOMPARE(158, pager.getItemsCount());
    QCOMPARE(40, pager.getNumPages());
    QCOMPARE(4, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(1, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    QCOMPARE(range, pager.getRange());

    // MidRange = even number
    pager = TPaginator(158, 4, 6);
    QCOMPARE(158, pager.getItemsCount());
    QCOMPARE(40, pager.getNumPages());
    QCOMPARE(4, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(6, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    range.append(2);
    range.append(3);
    range.append(4);
    QCOMPARE(range, pager.getRange());
}

void TestPaginator::testSetItemsCount()
{
    QList<int> range;
    TPaginator pager(158, 4, 5);

    // ItemsCount < 0
    pager.setItemsCount(-4);
    QCOMPARE(0, pager.getItemsCount());
    QCOMPARE(1, pager.getNumPages());
    QCOMPARE(4, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(5, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    QCOMPARE(range, pager.getRange());

    // ItemsCount = 0
    pager.setItemsCount(0);
    QCOMPARE(0, pager.getItemsCount());
    QCOMPARE(1, pager.getNumPages());
    QCOMPARE(4, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(5, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    QCOMPARE(range, pager.getRange());
}

void TestPaginator::testSetLimit()
{
    QList<int> range;
    TPaginator pager(158, 4, 5);

    // Limit < 1
    pager.setLimit(0);
    QCOMPARE(158, pager.getItemsCount());
    QCOMPARE(16, pager.getNumPages());
    QCOMPARE(10, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(5, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    range.append(2);
    range.append(3);
    QCOMPARE(range, pager.getRange());

    // Limit = 1
    pager.setLimit(1);
    QCOMPARE(158, pager.getItemsCount());
    QCOMPARE(158, pager.getNumPages());
    QCOMPARE(1, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(5, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    range.append(2);
    range.append(3);
    QCOMPARE(range, pager.getRange());
}

void TestPaginator::testSetMidRange()
{
    QList<int> range;
    TPaginator pager(158, 4, 5);

    // MidRange < 1
    pager.setMidRange(-16);
    QCOMPARE(158, pager.getItemsCount());
    QCOMPARE(40, pager.getNumPages());
    QCOMPARE(4, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(3, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    range.append(2);
    QCOMPARE(range, pager.getRange());

    // MidRange = 1
    pager.setMidRange(1);
    QCOMPARE(158, pager.getItemsCount());
    QCOMPARE(40, pager.getNumPages());
    QCOMPARE(4, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(1, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    QCOMPARE(range, pager.getRange());

    // MidRange = even number
    pager.setMidRange(8);
    QCOMPARE(158, pager.getItemsCount());
    QCOMPARE(40, pager.getNumPages());
    QCOMPARE(4, pager.getLimit());
    QCOMPARE(0, pager.getOffset());
    QCOMPARE(8, pager.getMidRange());
    range = QList<int>();
    range.append(1);
    range.append(2);
    range.append(3);
    range.append(4);
    range.append(5);
    QCOMPARE(range, pager.getRange());
}

void TestPaginator::testSetCurrentPage()
{
    QList<int> range;
    TPaginator pager(158, 4, 5);

    // Page < 1
    pager.setCurrentPage(-53);
    QCOMPARE(1, pager.getCurrentPage());
    QCOMPARE(1, pager.getFirstPage());
    QCOMPARE(1, pager.getPreviousPage());
    QCOMPARE(2, pager.getNextPage());
    QCOMPARE(40, pager.getLastPage());
    QCOMPARE(true, pager.haveToPaginate());
    QCOMPARE(false, pager.isFirstPageEnabled());
    QCOMPARE(false, pager.hasPreviousPage());
    QCOMPARE(true, pager.hasNextPage());
    QCOMPARE(true, pager.isLastPageEnabled());
    range = QList<int>();
    range.append(1);// Current Page
    range.append(2);
    range.append(3);
    QCOMPARE(range, pager.getRange());

    // Page = 1
    pager.setCurrentPage(1);
    QCOMPARE(1, pager.getCurrentPage());
    QCOMPARE(1, pager.getFirstPage());
    QCOMPARE(1, pager.getPreviousPage());
    QCOMPARE(2, pager.getNextPage());
    QCOMPARE(40, pager.getLastPage());
    QCOMPARE(true, pager.haveToPaginate());
    QCOMPARE(false, pager.isFirstPageEnabled());
    QCOMPARE(false, pager.hasPreviousPage());
    QCOMPARE(true, pager.hasNextPage());
    QCOMPARE(true, pager.isLastPageEnabled());
    range = QList<int>();
    range.append(1);// Current Page
    range.append(2);
    range.append(3);
    QCOMPARE(range, pager.getRange());

    // Page <= floor(MidRange / 2)
    pager.setCurrentPage(2);
    QCOMPARE(2, pager.getCurrentPage());
    QCOMPARE(1, pager.getFirstPage());
    QCOMPARE(1, pager.getPreviousPage());
    QCOMPARE(3, pager.getNextPage());
    QCOMPARE(40, pager.getLastPage());
    QCOMPARE(true, pager.haveToPaginate());
    QCOMPARE(true, pager.isFirstPageEnabled());
    QCOMPARE(true, pager.hasPreviousPage());
    QCOMPARE(true, pager.hasNextPage());
    QCOMPARE(true, pager.isLastPageEnabled());
    range = QList<int>();
    range.append(1);
    range.append(2);// Current Page
    range.append(3);
    range.append(4);
    QCOMPARE(range, pager.getRange());

    // Page > floor(MidRange / 2) && Page <= NB.Pages - floor(MidRange / 2)
    pager.setCurrentPage(25);
    QCOMPARE(25, pager.getCurrentPage());
    QCOMPARE(1, pager.getFirstPage());
    QCOMPARE(24, pager.getPreviousPage());
    QCOMPARE(26, pager.getNextPage());
    QCOMPARE(40, pager.getLastPage());
    QCOMPARE(true, pager.haveToPaginate());
    QCOMPARE(true, pager.isFirstPageEnabled());
    QCOMPARE(true, pager.hasPreviousPage());
    QCOMPARE(true, pager.hasNextPage());
    QCOMPARE(true, pager.isLastPageEnabled());
    range = QList<int>();
    range.append(23);
    range.append(24);
    range.append(25);// Current Page
    range.append(26);
    range.append(27);
    QCOMPARE(range, pager.getRange());

    // Page > NB.Pages - floor(MidRange / 2)
    pager.setCurrentPage(39);
    QCOMPARE(39, pager.getCurrentPage());
    QCOMPARE(1, pager.getFirstPage());
    QCOMPARE(38, pager.getPreviousPage());
    QCOMPARE(40, pager.getNextPage());
    QCOMPARE(40, pager.getLastPage());
    QCOMPARE(true, pager.haveToPaginate());
    QCOMPARE(true, pager.isFirstPageEnabled());
    QCOMPARE(true, pager.hasPreviousPage());
    QCOMPARE(true, pager.hasNextPage());
    QCOMPARE(true, pager.isLastPageEnabled());
    range = QList<int>();
    range.append(37);
    range.append(38);
    range.append(39);// Current Page
    range.append(40);
    QCOMPARE(range, pager.getRange());

    // Page = NB.Pages
    pager.setCurrentPage(40);
    QCOMPARE(40, pager.getCurrentPage());
    QCOMPARE(1, pager.getFirstPage());
    QCOMPARE(39, pager.getPreviousPage());
    QCOMPARE(40, pager.getNextPage());
    QCOMPARE(40, pager.getLastPage());
    QCOMPARE(true, pager.haveToPaginate());
    QCOMPARE(true, pager.isFirstPageEnabled());
    QCOMPARE(true, pager.hasPreviousPage());
    QCOMPARE(false, pager.hasNextPage());
    QCOMPARE(false, pager.isLastPageEnabled());
    range = QList<int>();
    range.append(38);
    range.append(39);
    range.append(40);// Current Page
    QCOMPARE(range, pager.getRange());

    // Page > NB.Pages
    pager.setCurrentPage(59);
    QCOMPARE(1, pager.getCurrentPage());
    QCOMPARE(1, pager.getFirstPage());
    QCOMPARE(1, pager.getPreviousPage());
    QCOMPARE(2, pager.getNextPage());
    QCOMPARE(40, pager.getLastPage());
    QCOMPARE(true, pager.haveToPaginate());
    QCOMPARE(false, pager.isFirstPageEnabled());
    QCOMPARE(false, pager.hasPreviousPage());
    QCOMPARE(true, pager.hasNextPage());
    QCOMPARE(true, pager.isLastPageEnabled());
    range = QList<int>();
    range.append(1);// Current Page
    range.append(2);
    range.append(3);
    QCOMPARE(range, pager.getRange());
}

void TestPaginator::testIsValidPage()
{
    TPaginator pager(158, 4, 5);
    QCOMPARE(false, pager.isValidPage(-2));
    QCOMPARE(true, pager.isValidPage(1));
    QCOMPARE(true, pager.isValidPage(36));
    QCOMPARE(true, pager.isValidPage(40));
    QCOMPARE(false, pager.isValidPage(68));
}

//TF_TEST_SQLLESS_MAIN(TestPaginator)
TF_TEST_MAIN(TestPaginator)
#include "paginator.moc"
