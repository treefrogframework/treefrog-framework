#include <TfTest/TfTest>
#include "tsharedmemoryhash.h"
#include "tmalloc.h"
#include "tglobal.h"
// #include <list>
// #include <vector>
// #include <unordered_map>
//#include <QByteArray>


const QString listpath = "/test.list";
const QString mappath = "/test.map";


class TestSharedMemoryHash : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase() {}
    void testAlloc1();
    void testAlloc2();

    void bench();
};


void TestSharedMemoryHash::initTestCase()
{

}


void TestSharedMemoryHash::testAlloc1()
{
    TSharedMemoryHash smhash("/sharedhash.shm", 1024 * 1024);

    //Vector *vec = Tf::createContainer<Vector>(listpath, 1024 * 1024);

    int num = smhash.count();
    {
        qDebug() << "smhash count()" << num;
        smhash.insert("hoge", "hogehoge");
        smhash.insert("foo", "foofoo");
        qDebug() << "smhash count()" << smhash.count();

        qDebug() << "smhash.value(\"hoge\") =" << smhash.value("hoge");
        qDebug() << "smhash.value(\"foo\") =" << smhash.value("foo");

        smhash.insert("hoge", "foofoo");
        qDebug() << "smhash.value(\"hoge\") =" << smhash.value("hoge");
        Tf::memdump();
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
    int num = Tf::nblocks();
    {
        /*
        std::unordered_map<QByteArray, Value, std::hash<QByteArray>,
            std::equal_to<QByteArray>, TSharedMemoryAllocator<std::pair<const QByteArray, Value>, "/map"> > map;

        for (int i = 1000; i < 2000; i++) {
            map.insert_or_assign(QByteArray::number(i), Value{i, QByteArray::number(Tf::random(1000, 1999))});
        }

        QBENCHMARK {
            for (int i = 0; i < 100000; i++) {
                int idx = Tf::random(1000, 1999);
                map.insert_or_assign(QByteArray::number(idx), Value{i, QByteArray::number(Tf::random(1000, 1999))});
            }
        }
        */
    }

    QCOMPARE(Tf::nblocks(), num);
}

TF_TEST_SQLLESS_MAIN(TestSharedMemoryHash)
#include "sharedmemoryhash.moc"
