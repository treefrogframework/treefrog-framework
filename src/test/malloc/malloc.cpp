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
    void testAlloc6();
    void testAlloc7();
    void testAlloc8();
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
    alloc->dump();
    QCOMPARE(alloc->countBlocks(), 0);
    QCOMPARE(alloc->countFreeBlocks(), 0);
    QCOMPARE(alloc->dataSegmentSize(), 0U);
}

void TestMalloc::testAlloc2()
{
    void *p1 = alloc->malloc(100);
    QVERIFY(p1);
    void *p2 = alloc->malloc(200);
    QVERIFY(p2);

    QCOMPARE(alloc->countBlocks(), 2);
    QCOMPARE(alloc->countFreeBlocks(), 0);
    alloc->free(p2);
    QCOMPARE(alloc->countBlocks(), 1);
    QCOMPARE(alloc->countFreeBlocks(), 0);
    alloc->free(p1);
    alloc->dump();
    QCOMPARE(alloc->countBlocks(), 0);
    QCOMPARE(alloc->countFreeBlocks(), 0);
    QCOMPARE(alloc->dataSegmentSize(), 0U);
}

void TestMalloc::testAlloc3()
{
    void *p1 = alloc->malloc(100);
    QVERIFY(p1);
    void *p2 = alloc->malloc(200);
    QVERIFY(p2);

    alloc->free(p1);
    alloc->dump();
    QCOMPARE(alloc->countBlocks(), 2);
    QCOMPARE(alloc->countFreeBlocks(), 1);
    alloc->free(p2);
    alloc->dump();
    QCOMPARE(alloc->countBlocks(), 0);
    QCOMPARE(alloc->countFreeBlocks(), 0);
    QCOMPARE(alloc->dataSegmentSize(), 0U);
}

void TestMalloc::testAlloc4()
{
    void *p1 = alloc->malloc(128);
    QVERIFY(p1);
    void *p2 = alloc->malloc(224);
    QVERIFY(p2);
    void *p3 = alloc->malloc(320);
    QVERIFY(p3);

    alloc->free(p2);
    alloc->dump();
    QCOMPARE(alloc->countBlocks(), 3);
    QCOMPARE(alloc->countFreeBlocks(), 1);
    QCOMPARE(alloc->sizeOfFreeBlocks(), 224U);
    alloc->free(p1);
    alloc->dump();
    QCOMPARE(alloc->countBlocks(), 2);
    QCOMPARE(alloc->countFreeBlocks(), 1);
#if defined(Q_PROCESSOR_X86_32) || defined(Q_PROCESSOR_ARM_32)
    QCOMPARE(alloc->sizeOfFreeBlocks(), 224U + 16 + 128);
#else
    QCOMPARE(alloc->sizeOfFreeBlocks(), 224U + 24 + 128);
#endif
    alloc->free(p3);
    alloc->dump();
    QCOMPARE(alloc->countBlocks(), 0);
    QCOMPARE(alloc->countFreeBlocks(), 0);
    QCOMPARE(alloc->dataSegmentSize(), 0U);
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
    QCOMPARE(alloc->countBlocks(), 3);
    QCOMPARE(alloc->countFreeBlocks(), 1);
    alloc->free(p2);
    QCOMPARE(alloc->countBlocks(), 2);
    QCOMPARE(alloc->countFreeBlocks(), 1);
    alloc->free(p3);
    alloc->dump();
    QCOMPARE(alloc->countBlocks(), 0);
    QCOMPARE(alloc->countFreeBlocks(), 0);
    QCOMPARE(alloc->dataSegmentSize(), 0U);
}

void TestMalloc::testAlloc6()
{
    void *p1 = alloc->malloc(1024);
    void *p2 = alloc->malloc(64);
    alloc->free(p1);
    void *p3 = alloc->malloc(32);

    alloc->dump();
    QCOMPARE(alloc->countBlocks(), 3);
    QCOMPARE(alloc->countFreeBlocks(), 1);
    alloc->free(p3);
    QCOMPARE(alloc->countBlocks(), 2);
    QCOMPARE(alloc->countFreeBlocks(), 1);
    alloc->free(p2);
    alloc->dump();
    QCOMPARE(alloc->countBlocks(), 0);
    QCOMPARE(alloc->countFreeBlocks(), 0);
    QCOMPARE(alloc->dataSegmentSize(), 0U);
}

void TestMalloc::testAlloc7()
{
    void *p1 = alloc->malloc(1024);
    void *p2 = alloc->malloc(70);
    void *p3 = alloc->malloc(32);
    alloc->free(p1);
    p1 = alloc->malloc(980);
    QCOMPARE(alloc->countBlocks(), 3);
    QCOMPARE(alloc->countFreeBlocks(), 0);
    alloc->free(p1);
    QCOMPARE(alloc->countBlocks(), 3);
    QCOMPARE(alloc->countFreeBlocks(), 1);
    alloc->free(p2);
    alloc->dump();
    QCOMPARE(alloc->countBlocks(), 2);
    QCOMPARE(alloc->countFreeBlocks(), 1);
    p1 = alloc->malloc(1024 + 70 + 1);
    alloc->dump();
    QCOMPARE(alloc->countBlocks(), 2);
    QCOMPARE(alloc->countFreeBlocks(), 0);
    alloc->free(p1);
    QCOMPARE(alloc->countBlocks(), 2);
    QCOMPARE(alloc->countFreeBlocks(), 1);
    alloc->free(p3);
    QCOMPARE(alloc->countBlocks(), 0);
    QCOMPARE(alloc->countFreeBlocks(), 0);
    QCOMPARE(alloc->dataSegmentSize(), 0U);
}

void TestMalloc::testAlloc8()
{
    void *p1 = alloc->malloc(1024);
    void *p2 = alloc->malloc(64);
    void *p3 = alloc->malloc(32);
    void *p4 = alloc->malloc(32);
    alloc->free(p1);
    alloc->free(p3);
    alloc->free(p2);
    alloc->dump();
    QCOMPARE(alloc->countBlocks(), 2);
    QCOMPARE(alloc->countFreeBlocks(), 1);
#if defined(Q_PROCESSOR_X86_32) || defined(Q_PROCESSOR_ARM_32)
    QCOMPARE(alloc->sizeOfFreeBlocks(), 1024U + 16 + 64 + 16 + 32);
#else
    QCOMPARE(alloc->sizeOfFreeBlocks(), 1024U + 24 + 64 + 24 + 32);
#endif
    alloc->free(p4);
    QCOMPARE(alloc->countBlocks(), 0);
    QCOMPARE(alloc->countFreeBlocks(), 0);
    QCOMPARE(alloc->dataSegmentSize(), 0U);
}

void TestMalloc::testReuse1()
{
    void *p1 = alloc->malloc(1000);
    QVERIFY(p1);
    void *p2 = alloc->malloc(200);
    QVERIFY(p2);

    alloc->free(p1);
    alloc->dump();
    QCOMPARE(alloc->countBlocks(), 2);
    QCOMPARE(alloc->countFreeBlocks(), 1);
    void *p3 = alloc->malloc(900);
    alloc->dump();
    QCOMPARE(p1, p3);
    QCOMPARE(alloc->countBlocks(), 2);
    QCOMPARE(alloc->countFreeBlocks(), 0);
    alloc->free(p3);
    alloc->free(p2);
    QCOMPARE(alloc->countBlocks(), 0);
    QCOMPARE(alloc->countFreeBlocks(), 0);
    QCOMPARE(alloc->dataSegmentSize(), 0U);
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

    QVERIFY(alloc->countBlocks() > 0);
    // cleanup
    for (int i = 0; i < NUM; i++) {
        alloc->free(ptr[i]);
    }
    QCOMPARE(alloc->countBlocks(), 0);
}


TF_TEST_MAIN(TestMalloc)
#include "malloc.moc"
