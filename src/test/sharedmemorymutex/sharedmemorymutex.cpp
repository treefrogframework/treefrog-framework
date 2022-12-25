#include <TfTest/TfTest>
#include <QElapsedTimer>
#include "tsharedmemorykvs.h"
#include "tcommandlineinterface.h"
#include "tglobal.h"


static QByteArray randomString(int length)
{
    const auto ch = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-+/^=_[]@:;!#$%()~? \t\n";
    QByteArray ret;
    int max = (int)strlen(ch) - 1;

    for (int i = 0; i < length; ++i) {
        ret += ch[Tf::random(max)];
    }
    return ret;
}


void insertItems()
{
    TSharedMemoryKvs smhash;
    QByteArray key, value, val;

    while (smhash.tableSize() < 10000 || smhash.loadFactor() < 0.75) {
        key = QUuid::createUuid().toByteArray();
        value = randomString(Tf::random(16, 128));

        bool ok = smhash.set(key, value, 100);
        assert(ok);
        if (!ok) {
            break;
        }

        key = QUuid::createUuid().toByteArray();
        ok = smhash.set(key, value, 100);
        assert(ok);
        ok = smhash.remove(key);
        assert(ok);
    }
}


void test0(int d = 100000)
{
    TSharedMemoryKvs smhash;
    qDebug() << "test0 #0 smhash count:" << smhash.count() << "table size:" << smhash.tableSize();

    // insert data
    insertItems();
    qDebug() << "test0 #1 smhash count:" << smhash.count() << "table size:" << smhash.tableSize();

    QByteArray key, value, val;
    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < d; i++) {
        key = QUuid::createUuid().toByteArray();
        value = randomString(Tf::random(16, 128));

        bool ok = smhash.set(key, value, 100);
        assert(ok);
        std::this_thread::yield();
        val = smhash.get(key);
        assert(val == value);
        std::this_thread::yield();
        ok = smhash.remove(key);
        assert(ok);
        val = smhash.get(key);
        assert(val == QByteArray());
        std::this_thread::yield();
    }

    qDebug() << "test0 #2 smhash count:" << smhash.count() << "table size:" << smhash.tableSize();
    qDebug() << "test0 ... done  "  << timer.elapsed() / 1000.0 << "sec elapsed\n";
}


void test1(int d = 5000000)
{
    TSharedMemoryKvs smhash;
    qDebug() << "test1 #0 smhash count:" << smhash.count() << "table size:" << smhash.tableSize();

    // insert data
    insertItems();
    qDebug() << "test1 #1 smhash count:" << smhash.count() << "table size:" << smhash.tableSize();

    QByteArray key, value;
    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < d; i++) {
        key = QUuid::createUuid().toByteArray();
        value = smhash.get(key);
        assert(value == QByteArray());
    }

    qDebug() << "test1 ... done  "  << timer.elapsed() / 1000.0 << "sec elapsed\n";
}


static int command()
{
    int num = 0;

    if (QCoreApplication::arguments().count() > 1) {
        num = QCoreApplication::arguments().at(1).toInt();
    }

    TSharedMemoryKvs::initialize("tfcache.shm", "MEMORY_SIZE=256M");

    switch (num) {
    case 0:
        test0();
        break;
    case 1:
        test1();
    default:
        break;
    }

    TSharedMemoryKvs smhash;
    smhash.cleanup();
    return 0;
}


TF_CLI_MAIN(command)
