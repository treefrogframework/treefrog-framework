#include <TfTest/TfTest>
#include "tfmalloc.h"
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
    void *p1 = Tf::smalloc(100);
    QVERIFY(p1);

    Tf::sfree(p1);
    QCOMPARE(Tf::shmblocks(), 0);
    Tf::shmdump();
}

void TestMalloc::testAlloc2()
{
    void *p1 = Tf::smalloc(100);
    QVERIFY(p1);
    void *p2 = Tf::smalloc(200);
    QVERIFY(p2);

    Tf::sfree(p2);
    Tf::sfree(p1);
    QCOMPARE(Tf::shmblocks(), 0);
    Tf::shmdump();
}

void TestMalloc::testAlloc3()
{
    void *p1 = Tf::smalloc(100);
    QVERIFY(p1);
    void *p2 = Tf::smalloc(200);
    QVERIFY(p2);

    Tf::sfree(p1);
    Tf::shmdump();
    Tf::sfree(p2);
    QCOMPARE(Tf::shmblocks(), 0);
    Tf::shmdump();
}

void TestMalloc::testAlloc4()
{
    void *p1 = Tf::smalloc(100);
    QVERIFY(p1);
    void *p2 = Tf::smalloc(200);
    QVERIFY(p2);
    void *p3 = Tf::smalloc(300);
    QVERIFY(p3);

    Tf::sfree(p2);
    Tf::sfree(p1);
    Tf::shmdump();
    Tf::sfree(p3);
    QCOMPARE(Tf::shmblocks(), 0);
    Tf::shmdump();
}

void TestMalloc::testAlloc5()
{
    void *p1 = Tf::smalloc(100);
    QVERIFY(p1);
    void *p2 = Tf::smalloc(200);
    QVERIFY(p2);
    void *p3 = Tf::smalloc(300);
    QVERIFY(p3);

    Tf::sfree(p1);
    Tf::sfree(p2);
    Tf::sfree(p3);
    QCOMPARE(Tf::shmblocks(), 0);
    Tf::shmdump();
}


void TestMalloc::testReuse1()
{
    void *p1 = Tf::smalloc(1000);
    QVERIFY(p1);
    void *p2 = Tf::smalloc(200);
    QVERIFY(p2);

    Tf::sfree(p1);
    QVERIFY(Tf::shmblocks() > 0);
    Tf::shmdump();
    void *p3 = Tf::smalloc(800);
    QCOMPARE(p1, p3);
    QCOMPARE(Tf::shmblocks(), 2);
    Tf::shmdump();
    Tf::sfree(p3);
    Tf::sfree(p2);
    QCOMPARE(Tf::shmblocks(), 0);
}


void TestMalloc::bench()
{
    constexpr int NUM = 1024 * 16;
    void *ptr[NUM] = {nullptr};

    for (int i = 0; i < NUM / 2; i++) {
        ptr[i * 2] = Tf::smalloc(Tf::random(128, 1024));  // half & half
        QVERIFY(ptr[i * 2]);
    }

    QBENCHMARK {
        for (int i = 0; i < 10000; i++) {
            int d = Tf::random(0, NUM - 1);
            if (ptr[d]) {
                Tf::sfree(ptr[d]);
                ptr[d] = nullptr;
            } else {
                ptr[d] = Tf::smalloc(Tf::random(128, 1024));
                QVERIFY(ptr[d]);
            }
        }
    }

    QVERIFY(Tf::shmblocks() > 0);
    // cleanup
    for (int i = 0; i < NUM; i++) {
        Tf::sfree(ptr[i]);
    }
    QCOMPARE(Tf::shmblocks(), 0);
}


TF_TEST_SQLLESS_MAIN(TestMalloc)
#include "malloc.moc"
