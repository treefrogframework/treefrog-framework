#include <TfTest/TfTest>
#include "tsharedmemory.h"
#include "tglobal.h"
#include <iostream>


namespace {
TSharedMemory sharedMomory("testshared.shm");
}


class TestSharedMemory : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    void attach();
    void test1();
    void test2_data();
    void test2();
};


static QByteArray randomString(int length)
{
    constexpr auto ch = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-+/^=_[]@:;!#$%()~? \t\n";
    QByteArray ret;
    int max = (int)strlen(ch) - 1;

    for (int i = 0; i < length; ++i) {
        ret += ch[Tf::random(max)];
    }
    return ret;
}

void TestSharedMemory::initTestCase()
{
    sharedMomory.unlink();
    sharedMomory.create(100 * 1024 * 1024);
}

void TestSharedMemory::cleanupTestCase()
{
    sharedMomory.unlink();
}

void TestSharedMemory::attach()
{
    bool res = sharedMomory.attach();
    Q_ASSERT(res);
    Tf::msleep(1000);
    res = sharedMomory.detach();
    Q_ASSERT(res);
    Tf::msleep(1000);
    res = sharedMomory.attach();
    Q_ASSERT(res);
    res = sharedMomory.detach();
    Q_ASSERT(res);
}

void TestSharedMemory::test1()
{
    sharedMomory.attach();
    QVERIFY(sharedMomory.size() == 100 * 1024 * 1024);

    // Lock and unlock
    auto ptr = sharedMomory.data();
    QVERIFY(ptr != nullptr);
    bool res = sharedMomory.lockForRead();
    Q_ASSERT(res);
    res = sharedMomory.lockForWrite();
    Q_ASSERT(!res);
    res = sharedMomory.unlock();
    Q_ASSERT(res);
    res = sharedMomory.lockForWrite();
    Q_ASSERT(res);
    res = sharedMomory.unlock();
    Q_ASSERT(res);
    QVERIFY(ptr == sharedMomory.data());
}

void TestSharedMemory::test2_data()
{
    QTest::addColumn<QByteArray>("string");

    QTest::newRow("1") << QUuid::createUuid().toByteArray();
    QTest::newRow("2") << QUuid::createUuid().toByteArray();
    QTest::newRow("3") << randomString(128);
    QTest::newRow("4") << randomString(256);
    QTest::newRow("5") << randomString(1024);
}

void TestSharedMemory::test2()
{
    QFETCH(QByteArray, string);

    bool res = sharedMomory.attach();
    Q_ASSERT(res);
    auto ptr = sharedMomory.data();
    res = sharedMomory.lockForWrite();
    Q_ASSERT(res);
    std::memcpy(ptr, string.data(), string.length());
    sharedMomory.unlock();
    res = sharedMomory.detach();
    Q_ASSERT(res);

    res = sharedMomory.attach();
    Q_ASSERT(res);
    res = sharedMomory.lockForRead();
    Q_ASSERT(res);
    int cmp = strncmp((char *)sharedMomory.data(), string.data(), string.length());
    Q_ASSERT(cmp == 0);
    sharedMomory.unlock();
}

TF_TEST_MAIN(TestSharedMemory)
#include "sharedmemory.moc"
