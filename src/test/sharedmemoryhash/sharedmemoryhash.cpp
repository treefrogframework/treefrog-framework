#include <TfTest/TfTest>
#include "tsharedmemoryhash.h"
#include "tfmalloc.h"
#include "tglobal.h"
#include <iostream>


static TSharedMemoryHash smhash("/sharedhash.shm", 256 * 1024 * 1024);
const QByteArray ckey = QUuid::createUuid().toByteArray();

class TestSharedMemoryHash : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase() {}
    void testAlloc1_data();
    void testAlloc1();
    void testAlloc2_data();
    void testAlloc2();
    void testAlloc3_data();
    void testAlloc3();
    void testAlloc4_data();
    void testAlloc4();

    void bench1();
    void bench2();
    void testCompareWithQMap();
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
    smhash.clear();
    QCOMPARE(smhash.count(), 0);
    smhash.insert(ckey, "value");
}


void TestSharedMemoryHash::testAlloc1_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("1") << QUuid::createUuid().toByteArray()
                       << QByteArray(u8"こんにちは");
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

    QCOMPARE(smhash.value(key).isEmpty(), true);  // empty
    smhash.insert(key, value);
    qDebug() << "smhash.value(" << key << ") =" << smhash.value(key);
    auto val = smhash.value(key);
    QCOMPARE(val, value);
}


void TestSharedMemoryHash::testAlloc2_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("1") << ckey
                       << QByteArray(u8"こんにちは");
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

    int num = smhash.count();
    smhash.insert(key, value);
    qDebug() << "smhash.value(" << key << ") =" << smhash.value(key);
    auto val = smhash.value(key);
    QCOMPARE(val, value);
    QCOMPARE(smhash.count(), num);
}


void TestSharedMemoryHash::testAlloc3_data()
{
    int d = 100;
    while (smhash.loadFactor() < 0.2) {
        if (!smhash.insert(QByteArray::number(Tf::random(1000, 1000 + d++)), randomString(128))) {
            break;
        }
        std::cout << smhash.loadFactor() << " " << smhash.count() << std::endl;
    }

    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("1") << QUuid::createUuid().toByteArray()
                       << QByteArray(u8"こんにちは");
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

    int num = smhash.count();
    QCOMPARE(smhash.value(key).isEmpty(), true);  // empty
    smhash.insert(key, value);
    qDebug() << "smhash.value(" << key << ") =" << smhash.value(key);
    smhash.remove(key);
    auto val = smhash.value(key, QByteArray("hoge"));
    QCOMPARE(val, QByteArray("hoge"));  // Default value
    QCOMPARE(smhash.count(), num);
}


void TestSharedMemoryHash::testAlloc4_data()
{
    int d = 100;
    while (smhash.loadFactor() < 0.2) {
        if (!smhash.insert(QByteArray::number(Tf::random(1000, 1000 + d++)), randomString(128))) {
            break;
        }
        std::cout << smhash.loadFactor() << " " << smhash.count() << std::endl;
    }

    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("1") << QUuid::createUuid().toByteArray()
                       << QByteArray(u8"こんにちは");
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << randomString(64);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << randomString(128);
    QTest::newRow("4") << QUuid::createUuid().toByteArray()
                       << randomString(256);
}


void TestSharedMemoryHash::testAlloc4()
{
    //
    // Insert and Take
    //
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);

    int num = smhash.count();
    QCOMPARE(smhash.value(key).isEmpty(), true);  // empty
    smhash.insert(key, value);
    qDebug() << "smhash.value(" << key << ") =" << smhash.value(key);
    smhash.take(key);
    auto val = smhash.value(key, QByteArray("hoge"));
    QCOMPARE(val, QByteArray("hoge"));  // Default value
    QCOMPARE(smhash.count(), num);
}


static void insert(QMap<QByteArray, QByteArray> &qmap, int count, float factor)
{
    QByteArray key, value;
    while (smhash.count() < count || smhash.loadFactor() < factor) {
        key = randomString(Tf::random(10, 20));
        value = randomString(Tf::random(16, 128));

        qmap.insert(key, value);

        bool ok = smhash.insert(key, value);
        QVERIFY(ok);
        if (!ok) {
            break;
        }
    }
}


void TestSharedMemoryHash::testCompareWithQMap()
{
    QMap<QByteArray, QByteArray> qmap;

    // insert data
    insert(qmap, 40000, 0.75);

    // check values
    for (auto it = qmap.constBegin(); it != qmap.constEnd(); ++it) {
        auto res = smhash.value(it.key());
        QCOMPARE(res, it.value());
    }

    QCOMPARE(smhash.count(), qmap.count());
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
        auto res = smhash.value(it.key());
        QCOMPARE(res, it.value());
    }

    QCOMPARE(smhash.count(), qmap.count());
    qDebug() << "QMap   count:" << qmap.count();
    qDebug() << "smhash count:" << smhash.count();

    // insert data
    insert(qmap, 40000, 0.75);

    // check values
    for (auto it = qmap.constBegin(); it != qmap.constEnd(); ++it) {
        auto res = smhash.value(it.key());
        QCOMPARE(res, it.value());
    }

    QCOMPARE(smhash.count(), qmap.count());
    qDebug() << "QMap   count:" << qmap.count();
    qDebug() << "smhash count:" << smhash.count();
    Tf::shmsummary();

    smhash.clear();
    QCOMPARE(smhash.count(), 0);
}


void TestSharedMemoryHash::bench1()
{
    int d = 100;
    while (smhash.loadFactor() < 0.75) {
        if (!smhash.insert(QByteArray::number(Tf::random(1000, 1000 + d++)), randomString(128))) {
            break;
        }
    }

    QBENCHMARK {
        for (int i = 0; i < 1000; i++) {
            int idx = Tf::random(1000, 1000 + d);
            smhash.value(QByteArray::number(idx));
        }
    }

    //qDebug() << "key range: [ 1000 -" << 1000 + d << "]";
    //qDebug() << "count:" << smhash.count() << " block num:" << Tf::nblocks();
    Tf::shmsummary();
    smhash.clear();
    QCOMPARE(smhash.count(), 0);
}


void TestSharedMemoryHash::bench2()
{
    int d = 100;
    while (smhash.loadFactor() < 0.75) {
        if (!smhash.insert(QByteArray::number(Tf::random(1000, 1000 + d++)), randomString(128))) {
            break;
        }
    }

    // Insert & take
    QByteArray res, value;
    QBENCHMARK {
        for (int i = 0; i < 1000; i++) {
            int idx = Tf::random(1000, 1000 + d);
            value =  randomString(128);
            bool ok = smhash.insert(QByteArray::number(idx), value);
            QVERIFY(ok);
            if (smhash.loadFactor() > 0.75) {
                res = smhash.take(QByteArray::number(idx));
            } else {
                res = smhash.value(QByteArray::number(idx));
            }
            QCOMPARE(res, value);
        }
    }

    //qDebug() << "key range: [ 1000 -" << 1000 + d << "]";
    //qDebug() << "count:" << smhash.count() << " block num:" << Tf::nblocks();
    Tf::shmsummary();
    smhash.clear();
    QCOMPARE(smhash.count(), 0);
}

TF_TEST_SQLLESS_MAIN(TestSharedMemoryHash)
#include "sharedmemoryhash.moc"
