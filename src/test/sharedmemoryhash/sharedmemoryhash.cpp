#include <TfTest/TfTest>
#include "tsharedmemoryhash.h"
#include "tfmalloc.h"
#include "tglobal.h"
// #include <list>
// #include <vector>
// #include <unordered_map>
//#include <QByteArray>


const QString listpath = "/test.list";
const QString mappath = "/test.map";
static TSharedMemoryHash smhash("/sharedhash.shm", 1024 * 1024);


class TestSharedMemoryHash : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase() {}
    void testAlloc1_data();
    void testAlloc1();
    void testAlloc2();

    void bench();
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

}

void TestSharedMemoryHash::testAlloc1_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    auto key = QUuid::createUuid().toByteArray();
    QTest::newRow("1") << key
                       << QByteArray(u8"こんにちは");
    QTest::newRow("2") << QUuid::createUuid().toByteArray()
                       << randomString(256);
    QTest::newRow("3") << QUuid::createUuid().toByteArray()
                       << randomString(256);
    QTest::newRow("4") << key
                       << randomString(256);
}

void TestSharedMemoryHash::testAlloc1()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);

    int num = smhash.count();
    {
        qDebug() << "smhash count()" << num;
        qDebug() << "smhash.value(" << key << ") =" << smhash.value(key);
        smhash.insert(key, value);
        auto val = smhash.value(key);
        QCOMPARE(val, value);
        //Tf::memdump();
        //Tf::msleep(100000);
    }
    //QCOMPARE(Tf::nblocks(), num);
}


void TestSharedMemoryHash::testAlloc2()
{

    int num = Tf::nblocks();
    {
        /*
        std::unordered_map<QByteArray, Value, std::hash<QByteArray>,
            std::equal_to<QByteArray>, TSharedMemoryAllocator<std::pair<const QByteArray, Value>, mappath> > map;

        qDebug() << "first time: map[\"hoge\"] =" << map["hoge"];
        map.insert_or_assign("hoge", Value{1, "hogehoge"});
        map.insert_or_assign("foo", Value{2, "foofoo"});
        map.insert_or_assign("hoge", Value{1, "foo"});
        qDebug() << "map[\"hoge\"] =" << map["hoge"];
        QCOMPARE(map["hoge"].second, "foo");
        Tf::memdump();
        //Tf::syncSharedMemory();
        //Tf::msleep(100000);
        */
    }
    QCOMPARE(Tf::nblocks(), num);

}


void TestSharedMemoryHash::bench()
{
    for (int i = 1000; i < 2000; i++) {
        smhash.insert(QByteArray::number(i), QByteArray::number(Tf::random(1000, 1999)));
    }

    QBENCHMARK {
        for (int i = 0; i < 100000; i++) {
            int idx = Tf::random(1000, 1999);
            smhash.insert(QByteArray::number(idx), QByteArray::number(Tf::random(1000, 1999)));
        }
    }

    qDebug() << "count:" << smhash.count() << "block num:" << Tf::nblocks();
    //smhash.clear();
    //qDebug() << "cleared. count:" << smhash.count() << "block num:" << Tf::nblocks();
    //Tf::memdump();
}

TF_TEST_SQLLESS_MAIN(TestSharedMemoryHash)
#include "sharedmemoryhash.moc"
