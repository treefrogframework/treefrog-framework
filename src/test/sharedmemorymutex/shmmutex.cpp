#include <TfTest/TfTest>
#include <QElapsedTimer>
#include "tsharedmemoryhash.h"
#include "tglobal.h"


static TSharedMemoryHash smhash("/shmtext.shm", 256 * 1024 * 1024);


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
    QByteArray key, value, val;
    while (smhash.tableSize() < 10000 || smhash.loadFactor() < 0.75) {
        key = QUuid::createUuid().toByteArray();
        value = randomString(Tf::random(16, 128));

        bool ok = smhash.insert(key, value);
        assert(ok);
        if (!ok) {
            break;
        }

        key = QUuid::createUuid().toByteArray();
        ok = smhash.insert(key, value);
        assert(ok);
        ok = smhash.remove(key);
        assert(ok);
    }
}


void test0(int d = 100000)
{
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

        bool ok = smhash.insert(key, value);
        assert(ok);
        std::this_thread::yield();
        val = smhash.value(key);
        assert(val == value);
        std::this_thread::yield();
        val = smhash.take(key);
        assert(val == value);
        val = smhash.value(key);
        assert(val == QByteArray());
        std::this_thread::yield();
    }

    qDebug() << "test0 #2 smhash count:" << smhash.count() << "table size:" << smhash.tableSize();
    qDebug() << "test0 ... done  "  << timer.elapsed() / 1000.0 << "sec elapsed\n";
}


void test1(int d = 5000000)
{
    qDebug() << "test1 #0 smhash count:" << smhash.count() << "table size:" << smhash.tableSize();

    // insert data
    insertItems();
    qDebug() << "test1 #1 smhash count:" << smhash.count() << "table size:" << smhash.tableSize();

    QByteArray key, value;
    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < d; i++) {
        key = QUuid::createUuid().toByteArray();
        value = smhash.value(key);
        assert(value == QByteArray());
    }

    qDebug() << "test1 ... done  "  << timer.elapsed() / 1000.0 << "sec elapsed\n";
}


int main(int argc, char *argv[])
{
    int num = 0;

    if (argc > 1) {
        num = atoi(argv[1]);
    }

    switch (num) {
    case 0:
        test0();
        break;
    case 1:
        test1();
    default:
        break;
    }
    return 0;
}
