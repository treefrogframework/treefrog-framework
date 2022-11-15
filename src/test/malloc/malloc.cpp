#include <TfTest/TfTest>
#include "tfmalloc.h"
#include "tglobal.h"


class TestMalloc : public QObject
{
    Q_OBJECT
    TSharedMemoryAllocator sha {"/testallocator.shm", 24 * 1024 * 1024};

private slots:
    void initTestCase();
    void cleanupTestCase() { QFile("/dev/shm/testallocator.shm").remove(); }
    void testAlloc1();
    void testAlloc2();
    void testAlloc3();
    void testAlloc4();
    void testAlloc5();
    void testReuse1();

    void bench();
};


void TestMalloc::initTestCase()
{}


void TestMalloc::testAlloc1()
{
    void *p1 = sha.malloc(100);
    QVERIFY(p1);

    sha.free(p1);
    QCOMPARE(sha.nblocks(), 0);
    sha.dump();
}

void TestMalloc::testAlloc2()
{
    void *p1 = sha.malloc(100);
    QVERIFY(p1);
    void *p2 = sha.malloc(200);
    QVERIFY(p2);

    sha.free(p2);
    sha.free(p1);
    QCOMPARE(sha.nblocks(), 0);
    sha.dump();
}

void TestMalloc::testAlloc3()
{
    void *p1 = sha.malloc(100);
    QVERIFY(p1);
    void *p2 = sha.malloc(200);
    QVERIFY(p2);

    sha.free(p1);
    sha.dump();
    sha.free(p2);
    QCOMPARE(sha.nblocks(), 0);
    sha.dump();
}

void TestMalloc::testAlloc4()
{
    void *p1 = sha.malloc(100);
    QVERIFY(p1);
    void *p2 = sha.malloc(200);
    QVERIFY(p2);
    void *p3 = sha.malloc(300);
    QVERIFY(p3);

    sha.free(p2);
    sha.free(p1);
    sha.dump();
    sha.free(p3);
    QCOMPARE(sha.nblocks(), 0);
    sha.dump();
}

void TestMalloc::testAlloc5()
{
    void *p1 = sha.malloc(100);
    QVERIFY(p1);
    void *p2 = sha.malloc(200);
    QVERIFY(p2);
    void *p3 = sha.malloc(300);
    QVERIFY(p3);

    sha.free(p1);
    sha.free(p2);
    sha.free(p3);
    QCOMPARE(sha.nblocks(), 0);
    sha.dump();
}


void TestMalloc::testReuse1()
{
    void *p1 = sha.malloc(1000);
    QVERIFY(p1);
    void *p2 = sha.malloc(200);
    QVERIFY(p2);

    sha.free(p1);
    QVERIFY(sha.nblocks() > 0);
    sha.dump();
    void *p3 = sha.malloc(800);
    QCOMPARE(p1, p3);
    QCOMPARE(sha.nblocks(), 2);
    sha.dump();
    sha.free(p3);
    sha.free(p2);
    QCOMPARE(sha.nblocks(), 0);
}


void TestMalloc::bench()
{
    constexpr int NUM = 1024 * 16;
    void *ptr[NUM] = {nullptr};

    for (int i = 0; i < NUM / 2; i++) {
        ptr[i * 2] = sha.malloc(Tf::random(128, 1024));  // half & half
        QVERIFY(ptr[i * 2]);
    }

    QBENCHMARK {
        for (int i = 0; i < 10000; i++) {
            int d = Tf::random(0, NUM - 1);
            if (ptr[d]) {
                sha.free(ptr[d]);
                ptr[d] = nullptr;
            } else {
                ptr[d] = sha.malloc(Tf::random(128, 1024));
                QVERIFY(ptr[d]);
            }
        }
    }

    QVERIFY(sha.nblocks() > 0);
    // cleanup
    for (int i = 0; i < NUM; i++) {
        sha.free(ptr[i]);
    }
    QCOMPARE(sha.nblocks(), 0);
}


TF_TEST_SQLLESS_MAIN(TestMalloc)
#include "malloc.moc"
