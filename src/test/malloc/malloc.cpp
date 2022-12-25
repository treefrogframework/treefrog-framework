#include <TfTest/TfTest>
#include "tsharedmemoryallocator.h"
#include "tglobal.h"


class TestMalloc : public QObject
{
    Q_OBJECT
    TSharedMemoryAllocator *alloc {nullptr};

private slots:
    void initTestCase();
    void cleanupTestCase();
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
    alloc = TSharedMemoryAllocator::initialize("alloctest.shm", 512 * 1024 * 1024);
}

void TestMalloc::cleanupTestCase()
{
    TSharedMemoryAllocator::unlink("alloctest.shm");
}

void TestMalloc::testAlloc1()
{
    void *p1 = alloc->malloc(100);
    QVERIFY(p1);

    alloc->free(p1);
    QCOMPARE(alloc->nblocks(), 0);
    alloc->dump();
}

void TestMalloc::testAlloc2()
{
    void *p1 = alloc->malloc(100);
    QVERIFY(p1);
    void *p2 = alloc->malloc(200);
    QVERIFY(p2);

    alloc->free(p2);
    alloc->free(p1);
    QCOMPARE(alloc->nblocks(), 0);
    alloc->dump();
}

void TestMalloc::testAlloc3()
{
    void *p1 = alloc->malloc(100);
    QVERIFY(p1);
    void *p2 = alloc->malloc(200);
    QVERIFY(p2);

    alloc->free(p1);
    alloc->dump();
    alloc->free(p2);
    QCOMPARE(alloc->nblocks(), 0);
    alloc->dump();
}

void TestMalloc::testAlloc4()
{
    void *p1 = alloc->malloc(100);
    QVERIFY(p1);
    void *p2 = alloc->malloc(200);
    QVERIFY(p2);
    void *p3 = alloc->malloc(300);
    QVERIFY(p3);

    alloc->free(p2);
    alloc->free(p1);
    alloc->dump();
    alloc->free(p3);
    QCOMPARE(alloc->nblocks(), 0);
    alloc->dump();
}

void TestMalloc::testAlloc5()
{
    void *p1 = alloc->malloc(100);
    QVERIFY(p1);
    void *p2 = alloc->malloc(200);
    QVERIFY(p2);
    void *p3 = alloc->malloc(300);
    QVERIFY(p3);

    alloc->free(p1);
    alloc->free(p2);
    alloc->free(p3);
    QCOMPARE(alloc->nblocks(), 0);
    alloc->dump();
}


void TestMalloc::testReuse1()
{
    void *p1 = alloc->malloc(1000);
    QVERIFY(p1);
    void *p2 = alloc->malloc(200);
    QVERIFY(p2);

    alloc->free(p1);
    QVERIFY(alloc->nblocks() > 0);
    alloc->dump();
    void *p3 = alloc->malloc(800);
    QCOMPARE(p1, p3);
    QCOMPARE(alloc->nblocks(), 2);
    alloc->dump();
    alloc->free(p3);
    alloc->free(p2);
    QCOMPARE(alloc->nblocks(), 0);
}


void TestMalloc::bench()
{
    constexpr int NUM = 1024 * 16;
    void *ptr[NUM] = {nullptr};

    for (int i = 0; i < NUM / 2; i++) {
        ptr[i * 2] = alloc->malloc(Tf::random(128, 1024));  // half & half
        QVERIFY(ptr[i * 2]);
    }

    QBENCHMARK {
        for (int i = 0; i < 10000; i++) {
            int d = Tf::random(0, NUM - 1);
            if (ptr[d]) {
                alloc->free(ptr[d]);
                ptr[d] = nullptr;
            } else {
                ptr[d] = alloc->malloc(Tf::random(128, 1024));
                QVERIFY(ptr[d]);
            }
        }
    }

    QVERIFY(alloc->nblocks() > 0);
    // cleanup
    for (int i = 0; i < NUM; i++) {
        alloc->free(ptr[i]);
    }
    QCOMPARE(alloc->nblocks(), 0);
}


TF_TEST_SQLLESS_MAIN(TestMalloc)
#include "malloc.moc"
