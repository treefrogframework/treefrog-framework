#include <TfTest/TfTest>
#include "../../tpaginator.h"

Q_DECLARE_METATYPE(QList<int>)

class TestPaginator : public QObject
{
    Q_OBJECT
private slots:
    void constructor_data();
    void constructor();
    void setItemsCount_data();
    void setItemsCount();
    void setLimit_data();
    void setLimit();
    void setMidRange_data();
    void setMidRange();
    void setCurrentPage_data();
    void setCurrentPage();
    void isValidPage_data();
    void isValidPage();
    void itemsCountChangesMakeCurrentPageInvalid_data();
    void itemsCountChangesMakeCurrentPageInvalid();
    void limitChangesMakeCurrentPageInvalid_data();
    void limitChangesMakeCurrentPageInvalid();
};

void TestPaginator::constructor_data()
{
    QTest::addColumn<TPaginator>("pager");
    QTest::addColumn<int>("expectedItemsCount");
    QTest::addColumn<int>("expectedNumPages");
    QTest::addColumn<int>("expectedLimit");
    QTest::addColumn<int>("expectedOffset");
    QTest::addColumn<int>("expectedMidRange");
    QTest::addColumn<QList<int> >("expectedRange");

    TPaginator pager;
    QList<int> range;
    range << 1;
    QTest::newRow("No args") << pager << 0 << 1 << 1 << 0 << 1 << range;

    pager = TPaginator(-9, 4, 5);
    range = QList<int>();
    range << 1;
    QTest::newRow("ItemsCount < 0") << pager << 0 << 1 << 4 << 0 << 5 << range;

    pager = TPaginator(0, 4, 5);
    range = QList<int>();
    range << 1;
    QTest::newRow("ItemsCount = 0") << pager << 0 << 1 << 4 << 0 << 5 << range;

    pager = TPaginator(158, -3, 5);
    range = QList<int>();
    range << 1 << 2 << 3;
    QTest::newRow("Limit < 1") << pager << 158 << 158 << 1 << 0 << 5 << range;

    pager = TPaginator(158, 1, 5);
    range = QList<int>();
    range << 1 << 2 << 3;
    QTest::newRow("Limit = 1") << pager << 158 << 158 << 1 << 0 << 5 << range;

    pager = TPaginator(158, 4, -98);
    range = QList<int>();
    range << 1;
    QTest::newRow("MidRange < 1") << pager << 158 << 40 << 4 << 0 << 1 << range;

    pager = TPaginator(158, 4, 1);
    range = QList<int>();
    range << 1;
    QTest::newRow("MidRange = 1") << pager << 158 << 40 << 4 << 0 << 1 << range;

    pager = TPaginator(158, 4, 6);
    range = QList<int>();
    range << 1 << 2 << 3 << 4;
    QTest::newRow("MidRange = 1") << pager << 158 << 40 << 4 << 0 << 7 << range;
}

void TestPaginator::constructor()
{
    QFETCH(TPaginator, pager);
    QFETCH(int, expectedItemsCount);
    QFETCH(int, expectedNumPages);
    QFETCH(int, expectedLimit);
    QFETCH(int, expectedOffset);
    QFETCH(int, expectedMidRange);
    QFETCH(QList<int>, expectedRange);

    QCOMPARE(pager.itemsCount(), expectedItemsCount);
    QCOMPARE(pager.numPages(), expectedNumPages);
    QCOMPARE(pager.limit(), expectedLimit);
    QCOMPARE(pager.offset(), expectedOffset);
    QCOMPARE(pager.midRange(), expectedMidRange);
    QCOMPARE(pager.range(), expectedRange);
}

void TestPaginator::setItemsCount_data()
{
    QTest::addColumn<TPaginator>("pager");
    QTest::addColumn<int>("itemsCount");
    QTest::addColumn<int>("expectedItemsCount");
    QTest::addColumn<int>("expectedNumPages");
    QTest::addColumn<int>("expectedLimit");
    QTest::addColumn<int>("expectedOffset");
    QTest::addColumn<int>("expectedMidRange");
    QTest::addColumn<QList<int> >("expectedRange");

    TPaginator pager(158, 4, 5);
    QList<int> range;
    range << 1;
    QTest::newRow("ItemsCount < 0") << pager << -4 << 0 << 1 << 4 << 0 << 5 << range;

    pager = TPaginator(158, 4, 5);
    range = QList<int>();
    range << 1;
    QTest::newRow("ItemsCount = 0") << pager << 0 << 0 << 1 << 4 << 0 << 5 << range;
}

void TestPaginator::setItemsCount()
{
    QFETCH(TPaginator, pager);
    QFETCH(int, itemsCount);
    QFETCH(int, expectedItemsCount);
    QFETCH(int, expectedNumPages);
    QFETCH(int, expectedLimit);
    QFETCH(int, expectedOffset);
    QFETCH(int, expectedMidRange);
    QFETCH(QList<int>, expectedRange);

    pager.setItemsCount(itemsCount);

    QCOMPARE(pager.itemsCount(), expectedItemsCount);
    QCOMPARE(pager.numPages(), expectedNumPages);
    QCOMPARE(pager.limit(), expectedLimit);
    QCOMPARE(pager.offset(), expectedOffset);
    QCOMPARE(pager.midRange(), expectedMidRange);
    QCOMPARE(pager.range(), expectedRange);
}

void TestPaginator::setLimit_data()
{
    QTest::addColumn<TPaginator>("pager");
    QTest::addColumn<int>("limit");
    QTest::addColumn<int>("expectedItemsCount");
    QTest::addColumn<int>("expectedNumPages");
    QTest::addColumn<int>("expectedLimit");
    QTest::addColumn<int>("expectedOffset");
    QTest::addColumn<int>("expectedMidRange");
    QTest::addColumn<QList<int> >("expectedRange");

    TPaginator pager(158, 4, 5);
    QList<int> range;
    range << 1 << 2 << 3;
    QTest::newRow("Limit < 1") << pager << 0 << 158 << 158 << 1 << 0 << 5 << range;

    pager = TPaginator(158, 4, 5);
    range = QList<int>();
    range << 1 << 2 << 3;
    QTest::newRow("Limit = 1") << pager << 1 << 158 << 158 << 1 << 0 << 5 << range;
}

void TestPaginator::setLimit()
{
    QFETCH(TPaginator, pager);
    QFETCH(int, limit);
    QFETCH(int, expectedItemsCount);
    QFETCH(int, expectedNumPages);
    QFETCH(int, expectedLimit);
    QFETCH(int, expectedOffset);
    QFETCH(int, expectedMidRange);
    QFETCH(QList<int>, expectedRange);

    pager.setLimit(limit);

    QCOMPARE(pager.itemsCount(), expectedItemsCount);
    QCOMPARE(pager.numPages(), expectedNumPages);
    QCOMPARE(pager.limit(), expectedLimit);
    QCOMPARE(pager.offset(), expectedOffset);
    QCOMPARE(pager.midRange(), expectedMidRange);
    QCOMPARE(pager.range(), expectedRange);
}

void TestPaginator::setMidRange_data()
{
    QTest::addColumn<TPaginator>("pager");
    QTest::addColumn<int>("midRange");
    QTest::addColumn<int>("expectedItemsCount");
    QTest::addColumn<int>("expectedNumPages");
    QTest::addColumn<int>("expectedLimit");
    QTest::addColumn<int>("expectedOffset");
    QTest::addColumn<int>("expectedMidRange");
    QTest::addColumn<QList<int> >("expectedRange");
    QTest::addColumn<int>("expectedCurrentPage");

    TPaginator pager(158, 4, 5);
    pager.setCurrentPage(20);
    QList<int> range;
    range << 20;
    QTest::newRow("MidRange < 1") << pager << -16 << 158 << 40 << 4 << 76 << 1 << range << 20;

    pager = TPaginator(158, 4, 5);
    pager.setCurrentPage(20);
    range = QList<int>();
    range << 20;
    QTest::newRow("MidRange = 1") << pager << 1 << 158 << 40 << 4 << 76 << 1 << range << 20;

    pager = TPaginator(158, 4, 5);
    pager.setCurrentPage(20);
    range = QList<int>();
    range << 16 << 17 << 18 << 19;
    range << 20;
    range << 21 << 22 << 23 << 24;
    QTest::newRow("MidRange = even number") << pager << 8 << 158 << 40 << 4 << 76 << 9 << range << 20;

    pager = TPaginator(158, 4, 5);
    pager.setCurrentPage(20);
    range = QList<int>();
    range << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12 << 13 << 14 << 15 << 16 << 17 << 18 << 19;
    range << 20;
    range << 21 << 22 << 23 << 24 << 25 << 26 << 27 << 28 << 29 << 30 << 31 << 32 << 33 << 34 << 35 << 36 << 37 << 38 << 39;
    QTest::newRow("MidRange = Nb.Pages - 1") << pager << 39 << 158 << 40 << 4 << 76 << 39 << range << 20;

    pager = TPaginator(158, 4, 5);
    pager.setCurrentPage(20);
    range = QList<int>();
    range << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12 << 13 << 14 << 15 << 16 << 17 << 18 << 19;
    range << 20;
    range << 21 << 22 << 23 << 24 << 25 << 26 << 27 << 28 << 29 << 30 << 31 << 32 << 33 << 34 << 35 << 36 << 37 << 38 << 39 << 40;
    QTest::newRow("MidRange = Nb.Pages && Nb.Pages = even number") << pager << 40 << 158 << 40 << 4 << 76 << 41 << range << 20;

    pager = TPaginator(158, 4, 5);
    pager.setCurrentPage(20);
    range = QList<int>();
    range << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12 << 13 << 14 << 15 << 16 << 17 << 18 << 19;
    range << 20;
    range << 21 << 22 << 23 << 24 << 25 << 26 << 27 << 28 << 29 << 30 << 31 << 32 << 33 << 34 << 35 << 36 << 37 << 38 << 39 << 40;
    QTest::newRow("MidRange > Nb.Pages") << pager << 53 << 158 << 40 << 4 << 76 << 53 << range << 20;

    pager = TPaginator(161, 4, 5);
    pager.setCurrentPage(21);
    range = QList<int>();
    range << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11 << 12 << 13 << 14 << 15 << 16 << 17 << 18 << 19 << 20;
    range << 21;
    range << 22 << 23 << 24 << 25 << 26 << 27 << 28 << 29 << 30 << 31 << 32 << 33 << 34 << 35 << 36 << 37 << 38 << 39 << 40 << 41;
    QTest::newRow("MidRange = Nb.Pages && Nb.Pages = odd number") << pager << 41 << 161 << 41 << 4 << 80 << 41 << range << 21;
}

void TestPaginator::setMidRange()
{
    QFETCH(TPaginator, pager);
    QFETCH(int, midRange);
    QFETCH(int, expectedItemsCount);
    QFETCH(int, expectedNumPages);
    QFETCH(int, expectedLimit);
    QFETCH(int, expectedOffset);
    QFETCH(int, expectedMidRange);
    QFETCH(QList<int>, expectedRange);
    QFETCH(int, expectedCurrentPage);

    pager.setMidRange(midRange);

    QCOMPARE(pager.itemsCount(), expectedItemsCount);
    QCOMPARE(pager.numPages(), expectedNumPages);
    QCOMPARE(pager.limit(), expectedLimit);
    QCOMPARE(pager.offset(), expectedOffset);
    QCOMPARE(pager.midRange(), expectedMidRange);
    QCOMPARE(pager.range(), expectedRange);
    QCOMPARE(pager.currentPage(), expectedCurrentPage);
}

void TestPaginator::setCurrentPage_data()
{
    QTest::addColumn<TPaginator>("pager");
    QTest::addColumn<int>("currentPage");
    QTest::addColumn<int>("expectedCurrentPage");
    QTest::addColumn<int>("expectedFirstPage");
    QTest::addColumn<int>("expectedPreviousPage");
    QTest::addColumn<int>("expectedNextPage");
    QTest::addColumn<int>("expectedLastPage");
    QTest::addColumn<bool>("expectedHaveToPaginate");
    QTest::addColumn<bool>("expectedIsFirstPageEnabled");
    QTest::addColumn<bool>("expectedHasPreviousPage");
    QTest::addColumn<bool>("expectedHasNextPage");
    QTest::addColumn<bool>("expectedIsLastPageEnabled");
    QTest::addColumn<QList<int> >("expectedRange");

    TPaginator pager(158, 4, 5);
    QList<int> range;
    range << 1 << 2 << 3;
    QTest::newRow("Page < 1") << pager << -53 << 1 << 1 << 1 << 2 << 40 << true << false << false << true << true << range;

    pager = TPaginator(158, 4, 5);
    range = QList<int>();
    range << 1 << 2 << 3;
    QTest::newRow("Page = 1") << pager << 1 << 1 << 1 << 1 << 2 << 40 << true << false << false << true << true << range;

    pager = TPaginator(158, 4, 5);
    range = QList<int>();
    range << 1 << 2 << 3 << 4;
    QTest::newRow("Page <= floor(MidRange / 2)") << pager << 2 << 2 << 1 << 1 << 3 << 40 << true << true << true << true << true << range;

    pager = TPaginator(158, 4, 5);
    range = QList<int>();
    range << 23 << 24 << 25 << 26 << 27;
    QTest::newRow("Page > floor(MidRange / 2) && Page <= Nb.Pages - floor(MidRange / 2)") << pager << 25 << 25 << 1 << 24 << 26 << 40 << true << true << true << true << true << range;

    pager = TPaginator(158, 4, 5);
    range = QList<int>();
    range << 37 << 38 << 39 << 40;
    QTest::newRow("Page > Nb.Pages - floor(MidRange / 2)") << pager << 39 << 39 << 1 << 38 << 40 << 40 << true << true << true << true << true << range;

    pager = TPaginator(158, 4, 5);
    range = QList<int>();
    range << 38 << 39 << 40;
    QTest::newRow("Page = Nb.Pages") << pager << 40 << 40 << 1 << 39 << 40 << 40 << true << true << true << false << false << range;

    pager = TPaginator(158, 4, 5);
    range = QList<int>();
    range << 1 << 2 << 3;
    QTest::newRow("Page > Nb.Pages") << pager << 59 << 1 << 1 << 1 << 2 << 40 << true << false << false << true << true << range;
}

void TestPaginator::setCurrentPage()
{
    QFETCH(TPaginator, pager);
    QFETCH(int, currentPage);
    QFETCH(int, expectedCurrentPage);
    QFETCH(int, expectedFirstPage);
    QFETCH(int, expectedPreviousPage);
    QFETCH(int, expectedNextPage);
    QFETCH(int, expectedLastPage);
    QFETCH(bool, expectedHaveToPaginate);
    QFETCH(bool, expectedIsFirstPageEnabled);
    QFETCH(bool, expectedHasPreviousPage);
    QFETCH(bool, expectedHasNextPage);
    QFETCH(bool, expectedIsLastPageEnabled);
    QFETCH(QList<int>, expectedRange);

    pager.setCurrentPage(currentPage);

    QCOMPARE(pager.currentPage(), expectedCurrentPage);
    QCOMPARE(pager.firstPage(), expectedFirstPage);
    QCOMPARE(pager.previousPage(), expectedPreviousPage);
    QCOMPARE(pager.nextPage(), expectedNextPage);
    QCOMPARE(pager.lastPage(), expectedLastPage);
    QCOMPARE(pager.haveToPaginate(), expectedHaveToPaginate);
    QCOMPARE(pager.isFirstPageEnabled(), expectedIsFirstPageEnabled);
    QCOMPARE(pager.hasPreviousPage(), expectedHasPreviousPage);
    QCOMPARE(pager.hasNextPage(), expectedHasNextPage);
    QCOMPARE(pager.isLastPageEnabled(), expectedIsLastPageEnabled);
    QCOMPARE(pager.range(), expectedRange);
}

void TestPaginator::isValidPage_data()
{
    QTest::addColumn<TPaginator>("pager");
    QTest::addColumn<int>("page");
    QTest::addColumn<bool>("expectedIsValidPage");

    TPaginator pager(158, 4, 5);
    QTest::newRow("Page < 1") << pager << -2 << false;
    QTest::newRow("Page = 1") << pager << 1 << true;
    QTest::newRow("Page = Valid page") << pager << 36 << true;
    QTest::newRow("Page = Last page") << pager << 40 << true;
    QTest::newRow("Page > Last page") << pager << 68 << false;
}

void TestPaginator::isValidPage()
{
    QFETCH(TPaginator, pager);
    QFETCH(int, page);
    QFETCH(bool, expectedIsValidPage);

    QCOMPARE(pager.isValidPage(page), expectedIsValidPage);
}

void TestPaginator::itemsCountChangesMakeCurrentPageInvalid_data()
{
    QTest::addColumn<TPaginator>("pager");
    QTest::addColumn<int>("itemsCount");
    QTest::addColumn<int>("expectedCurrentPage");

    TPaginator pager(158, 4, 5);
    pager.setCurrentPage(21);
    QTest::newRow("ItemsCount = Limit + 1") << pager << 5 << 1;
}

void TestPaginator::itemsCountChangesMakeCurrentPageInvalid()
{
    QFETCH(TPaginator, pager);
    QFETCH(int, itemsCount);
    QFETCH(int, expectedCurrentPage);

    pager.setItemsCount(itemsCount);

    QCOMPARE(pager.currentPage(), expectedCurrentPage);
}

void TestPaginator::limitChangesMakeCurrentPageInvalid_data()
{
    QTest::addColumn<TPaginator>("pager");
    QTest::addColumn<int>("limit");
    QTest::addColumn<int>("expectedCurrentPage");

    TPaginator pager(158, 4, 5);
    pager.setCurrentPage(21);
    QTest::newRow("Limit = ItemsCount - 1") << pager << 157 << 1;
    QTest::newRow("Limit > ItemsCount") << pager << 200 << 1;
}

void TestPaginator::limitChangesMakeCurrentPageInvalid()
{
    QFETCH(TPaginator, pager);
    QFETCH(int, limit);
    QFETCH(int, expectedCurrentPage);

    pager.setLimit(limit);

    QCOMPARE(pager.currentPage(), expectedCurrentPage);
}

//TF_TEST_SQLLESS_MAIN(TestPaginator)
TF_TEST_MAIN(TestPaginator)
#include "paginator.moc"
