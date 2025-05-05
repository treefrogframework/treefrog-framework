#include <TfTest/TfTest>
#include "tsharedmemorykvs.h"
#include "tglobal.h"
#include <iostream>


static QMap<QByteArray, QByteArray> qmap;
const QByteArray ckey = QUuid::createUuid().toByteArray();

class TestSharedMemoryHash : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void testAlloc1_data();
    void testAlloc1();
    void testAlloc2_data();
    void testAlloc2();
    void testAlloc3_data();
    void testAlloc3();
    void testAlloc4();

    void bench1();
    void bench2();
    void testCompareWithQMap();
    void testCompareIterator();
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


void TestSharedMemoryHash::initTestCase()
{
    TSharedMemoryKvs::initialize("tfcache.shm", "MEMORY_SIZE=256M");

    TSharedMemoryKvs smhash;
    smhash.clear();
    QCOMPARE(smhash.count(), 0U);
    smhash.set(ckey, "value", 10);
}


void TestSharedMemoryHash::cleanupTestCase()
{
    TSharedMemoryKvs smhash;
    smhash.cleanup();
}

void TestSharedMemoryHash::testAlloc1_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("1") << QUuid::createUuid().toByteArray()
                       << QByteArray("こんにちは");
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << randomString(64);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << randomString(128);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256);
}


void TestSharedMemoryHash::testAlloc1()
{
    //
    // Insert & Get
    //
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);

    TSharedMemoryKvs smhash;
    QCOMPARE(smhash.get(key).isEmpty(), true);  // empty
    smhash.set(key, value, 10);
    qDebug() << "smhash.get(" << key << ") =" << smhash.get(key);
    auto val = smhash.get(key);
    QCOMPARE(val, value);
}


void TestSharedMemoryHash::testAlloc2_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("1") << ckey
                       << QByteArray("こんにちは");
    QTest::newRow("2") << ckey
                       << randomString(64);
    QTest::newRow("3") << ckey
                       << randomString(128);
    QTest::newRow("4") << ckey
                       << randomString(256);
}


void TestSharedMemoryHash::testAlloc2()
{
    //
    // Insert by same key and get
    //
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);

    TSharedMemoryKvs smhash;
    uint num = smhash.count();
    smhash.set(key, value, 10);
    qDebug() << "smhash.get(" << key << ") =" << smhash.get(key);
    auto val = smhash.get(key);
    QCOMPARE(val, value);
    QCOMPARE(smhash.count(), num);
}


void TestSharedMemoryHash::testAlloc3_data()
{
    int d = 100;
    TSharedMemoryKvs smhash;

    while (smhash.loadFactor() < 0.2) {
        if (!smhash.set(QByteArray::number((uint)Tf::random(1000, 1000 + d++)), randomString(128), 10)) {
            break;
        }
        //std::cout << smhash.loadFactor() << " " << smhash.count() << std::endl;
    }

    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("1") << QUuid::createUuid().toByteArray()
                       << QByteArray("こんにちは");
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << randomString(64);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << randomString(128);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256);
}


void TestSharedMemoryHash::testAlloc3()
{
    //
    // Insert and Remove
    //
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);

    TSharedMemoryKvs smhash;
    uint num = smhash.count();
    QCOMPARE(smhash.get(key).isEmpty(), true);  // empty
    smhash.set(key, value, 10);
    qDebug() << "smhash.get(" << key << ") =" << smhash.get(key);
    smhash.remove(key);
    // auto val = smhash.get(key, QByteArray("hoge"));
    // QCOMPARE(val, QByteArray("hoge"));  // Default value
    QCOMPARE(smhash.count(), num);
}


void TestSharedMemoryHash::testAlloc4()
{
    //
    // Checks timeout
    //

    int d = 100;
    TSharedMemoryKvs smhash;

    while (smhash.loadFactor() < 0.2) {
        if (!smhash.set(QByteArray::number((uint)Tf::random(1000, 1000 + d++)), randomString(128), 100)) {
            break;
        }
    }

    qDebug() << "smhash count:" << smhash.count();

    for (int i = 0; i < 10; i++) {
        auto key = QUuid::createUuid().toByteArray();
        int seconds = Tf::random(1, 5);

        QCOMPARE(smhash.get(key), QByteArray());  // empty
        smhash.set(key, "hoge", seconds);
        QVERIFY(!smhash.get(key).isEmpty());  // not empty
        qDebug() << "smhash.get(" << key << ") =" << smhash.get(key);
        Tf::msleep(seconds * 1000 + 20);
        auto val = smhash.get(key);
        QCOMPARE(val, QByteArray());  // timeout, empty
    }
}


static void insert(uint count, float factor)
{
    QByteArray key, value;
    TSharedMemoryKvs smhash;

    while (smhash.count() < count || smhash.loadFactor() < factor) {
        key = randomString(Tf::random(10, 20));
        value = randomString(Tf::random(16, 128));

        qmap.insert(key, value);  // QMap

        bool ok = smhash.set(key, value, 1200);  // TSharedMemoryHash
        QVERIFY(ok);
        if (!ok) {
            break;
        }
    }
}


void TestSharedMemoryHash::testCompareWithQMap()
{
    // insert data
    insert(40000, 0.75);

    TSharedMemoryKvs smhash;

    // check values
    for (auto it = qmap.constBegin(); it != qmap.constEnd(); ++it) {
        auto res = smhash.get(it.key());
        QCOMPARE(res, it.value());
    }

    QCOMPARE(smhash.count(), (uint)qmap.count());
    qDebug() << "QMap   count:" << qmap.count();
    qDebug() << "smhash count:" << smhash.count();

    // remove values
    for (QMutableMapIterator<QByteArray, QByteArray> it(qmap); it.hasNext();) {
        it.next();
        smhash.remove(it.key()); // remove
        it.remove();

        if (smhash.count() < 10000) {
            break;
        }
    }

    // check values
    for (auto it = qmap.constBegin(); it != qmap.constEnd(); ++it) {
        auto res = smhash.get(it.key());
        QCOMPARE(res, it.value());
    }

    QCOMPARE(smhash.count(), (uint)qmap.count());
    qDebug() << "QMap   count:" << qmap.count();
    qDebug() << "smhash count:" << smhash.count();

    // insert data
    insert(40000, 0.75);

    // check values
    for (auto it = qmap.constBegin(); it != qmap.constEnd(); ++it) {
        auto res = smhash.get(it.key());
        QCOMPARE(res, it.value());
    }

    QCOMPARE(smhash.count(), (uint)qmap.count());
    qDebug() << "QMap   count:" << qmap.count();
    qDebug() << "smhash count:" << smhash.count();
    //Tf::shmsummary();

    smhash.clear();
    qmap.clear();
    QCOMPARE(smhash.count(), 0U);
}


void TestSharedMemoryHash::testCompareIterator()
{
    // insert data
    insert(40000, 0.75);

    TSharedMemoryKvs smhash;

    int cnt = 0;
    for (auto it = smhash.begin(); it != smhash.end(); ++it) {
        auto val = qmap.value(it.key());
        QCOMPARE(it.value(), val);
        cnt++;
    }
    QCOMPARE(cnt, qmap.count());

    for (auto it = qmap.begin(); it != qmap.end(); ++it) {
        auto val = smhash.get(it.key());
        QCOMPARE(it.value(), val);
    }

    // remove-check
    cnt = 0;
    for (auto it = smhash.begin(); it != smhash.end(); ++it) {
        auto val = qmap.value(it.key());
        if (val == it.value()) {
            it.remove();
            cnt++;
        }
    }
    QCOMPARE(cnt, qmap.count());
    QCOMPARE(smhash.count(), 0U);
    qmap.clear();
}


void TestSharedMemoryHash::bench1()
{
    int d = 100;
    TSharedMemoryKvs smhash;

    while (smhash.loadFactor() < 0.75 && smhash.tableSize() < 20000) {
        if (!smhash.set(QByteArray::number((uint)Tf::random(1000, 1000 + d++)), randomString(128), 10)) {
            break;
        }
    }

    QBENCHMARK {
        for (int i = 0; i < 1000; i++) {
            int idx = Tf::random(1000, 1000 + d);
            smhash.get(QByteArray::number(idx));
        }
    }

    //qDebug() << "key range: [ 1000 -" << 1000 + d << "]";
    //qDebug() << "count:" << smhash.count() << " block num:" << Tf::nblocks();
    //Tf::shmsummary();
    smhash.clear();
    QCOMPARE(smhash.count(), 0U);
}


void TestSharedMemoryHash::bench2()
{
    int d = 100;
    TSharedMemoryKvs smhash;

    while (smhash.loadFactor() < 0.75 && smhash.tableSize() < 20000) {
        if (!smhash.set(QByteArray::number((uint)Tf::random(1000, 1000 + d++)), randomString(128), 10)) {
            break;
        }
    }

    // set & remove
    QByteArray res, value;
    QBENCHMARK {
        for (int i = 0; i < 1000; i++) {
            int idx = Tf::random(1000, 1000 + d);
            value = randomString(128);
            bool ok = smhash.set(QByteArray::number(idx), value, 10);
            QVERIFY(ok);
            if (smhash.loadFactor() > 0.75) {
                ok = smhash.remove(QByteArray::number(idx));
                QVERIFY(ok);
            } else {
                res = smhash.get(QByteArray::number(idx));
                QCOMPARE(res, value);
            }
        }
    }

    //qDebug() << "key range: [ 1000 -" << 1000 + d << "]";
    //qDebug() << "count:" << smhash.count() << " block num:" << Tf::nblocks();
    //Tf::shmsummary();
    smhash.clear();
    QCOMPARE(smhash.count(), 0U);
}

TF_TEST_MAIN(TestSharedMemoryHash)
#include "sharedmemoryhash.moc"
