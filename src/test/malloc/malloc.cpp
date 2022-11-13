#include <TfTest/TfTest>
#include "tmalloc.h"
#include "tglobal.h"


class TestMalloc : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase() {}
    void testAlloc1();
    void testAlloc2();
    void testAlloc3();
    void testAlloc4();
    void testAlloc5();
    void testReuse1();

    void bench();
};


void TestMalloc::initTestCase()
{
    constexpr int SIZE = 256 * 1024 * 1024;
    void *p = new char[SIZE];
    Tf::setbrk(p, SIZE, true);
    qDebug() << "init:" << p;
}


void TestMalloc::testAlloc1()
{
    void *p1 = Tf::tmalloc(100);
    QVERIFY(p1);

    Tf::tfree(p1);
    QCOMPARE(Tf::nblocks(), 0);
    Tf::memdump();
}

void TestMalloc::testAlloc2()
{
    void *p1 = Tf::tmalloc(100);
    QVERIFY(p1);
    void *p2 = Tf::tmalloc(200);
    QVERIFY(p2);

    Tf::tfree(p2);
    Tf::tfree(p1);
    QCOMPARE(Tf::nblocks(), 0);
    Tf::memdump();
}

void TestMalloc::testAlloc3()
{
    void *p1 = Tf::tmalloc(100);
    QVERIFY(p1);
    void *p2 = Tf::tmalloc(200);
    QVERIFY(p2);

    Tf::tfree(p1);
    Tf::memdump();
    Tf::tfree(p2);
    QCOMPARE(Tf::nblocks(), 0);
    Tf::memdump();
}

void TestMalloc::testAlloc4()
{
    void *p1 = Tf::tmalloc(100);
    QVERIFY(p1);
    void *p2 = Tf::tmalloc(200);
    QVERIFY(p2);
    void *p3 = Tf::tmalloc(300);
    QVERIFY(p3);

    Tf::tfree(p2);
    Tf::tfree(p1);
    Tf::memdump();
    Tf::tfree(p3);
    QCOMPARE(Tf::nblocks(), 0);
    Tf::memdump();
}

void TestMalloc::testAlloc5()
{
    void *p1 = Tf::tmalloc(100);
    QVERIFY(p1);
    void *p2 = Tf::tmalloc(200);
    QVERIFY(p2);
    void *p3 = Tf::tmalloc(300);
    QVERIFY(p3);

    Tf::tfree(p1);
    Tf::tfree(p2);
    Tf::tfree(p3);
    QCOMPARE(Tf::nblocks(), 0);
    Tf::memdump();
}


void TestMalloc::testReuse1()
{
    void *p1 = Tf::tmalloc(1000);
    QVERIFY(p1);
    void *p2 = Tf::tmalloc(200);
    QVERIFY(p2);

    Tf::tfree(p1);
    QVERIFY(Tf::nblocks() > 0);
    Tf::memdump();
    void *p3 = Tf::tmalloc(800);
    QCOMPARE(p1, p3);
    QCOMPARE(Tf::nblocks(), 2);
    Tf::memdump();
    Tf::tfree(p3);
    Tf::tfree(p2);
    QCOMPARE(Tf::nblocks(), 0);
}


void TestMalloc::bench()
{
    constexpr int NUM = 1024 * 16;
    void *ptr[NUM] = {nullptr};

    for (int i = 0; i < NUM / 2; i++) {
        ptr[i * 2] = Tf::tmalloc(Tf::random(128, 1024));  // half & half
        QVERIFY(ptr[i * 2]);
    }

    QBENCHMARK {
        for (int i = 0; i < 10000; i++) {
            int d = Tf::random(0, NUM - 1);
            if (ptr[d]) {
                Tf::tfree(ptr[d]);
                ptr[d] = nullptr;
            } else {
                ptr[d] = Tf::tmalloc(Tf::random(128, 1024));
                QVERIFY(ptr[d]);
            }
        }
    }

    QVERIFY(Tf::nblocks() > 0);
    // cleanup
    for (int i = 0; i < NUM; i++) {
        Tf::tfree(ptr[i]);
    }
    QCOMPARE(Tf::nblocks(), 0);
}


TF_TEST_SQLLESS_MAIN(TestMalloc)
#include "malloc.moc"
