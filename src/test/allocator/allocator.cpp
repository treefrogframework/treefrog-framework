#include <TfTest/TfTest>
#include "tsharedmemoryallocator.h"
#include "tglobal.h"
#include <list>
#include <vector>
#include <unordered_map>
#include <QByteArray>


class TestAllocator : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase() {}
    void testAlloc1();
    void testAlloc2();


    void bench();
};


void TestAllocator::initTestCase()
{
    // qDebug() << Tf::nblocks();
    // if (Tf::nblocks() > 0) {
    //     _exit(0);
    // }
}


void TestAllocator::testAlloc1()
{
    {
        std::vector<int, TSharedMemoryAllocator<int>> v = {1, 2, 3};
        std::list<int, TSharedMemoryAllocator<int>> ls = {4, 5, 6};

        qDebug() << "v[1] =" << v[1];
        qDebug() << "ls.front() =" << ls.front();
        Tf::memdump();
    }
    QCOMPARE(Tf::nblocks(), 0);
}

void TestAllocator::testAlloc2()
{
    using Value = std::pair<qint64, QByteArray>;

    {
        std::unordered_map<QByteArray, Value, std::hash<QByteArray>,
            std::equal_to<QByteArray>, TSharedMemoryAllocator<std::pair<const QByteArray, Value>>> map;

        qDebug() << "first time: map[\"hoge\"] =" << map["hoge"];

        map.insert_or_assign("hoge", Value{1, "hogehoge"});
        map.insert_or_assign("foo", Value{2, "foofoo"});
        map.insert_or_assign("hoge", Value{1, "foo"});
        qDebug() << "map[\"hoge\"] =" << map["hoge"];
        QCOMPARE(map["hoge"].second, "foo");
        Tf::memdump();
        //Tf::syncSharedMemory();
        //Tf::msleep(100000);
    }
    QCOMPARE(Tf::nblocks(), 0);

}


void TestAllocator::bench()
{
    using Value = std::pair<qint64, QByteArray>;

    {
        std::unordered_map<QByteArray, Value, std::hash<QByteArray>,
            std::equal_to<QByteArray>, TSharedMemoryAllocator<std::pair<const QByteArray, Value>>> map;

        for (int i = 1000; i < 2000; i++) {
            map.insert_or_assign(QByteArray::number(i), Value{i, QByteArray::number(Tf::random(1000, 1999))});
        }

        QBENCHMARK {
            for (int i = 0; i < 100000; i++) {
                int idx = Tf::random(1000, 1999);
                map.insert_or_assign(QByteArray::number(idx), Value{i, QByteArray::number(Tf::random(1000, 1999))});
            }
        }
    }

    QCOMPARE(Tf::nblocks(), 0);
}


TF_TEST_SQLLESS_MAIN(TestAllocator)
#include "allocator.moc"
