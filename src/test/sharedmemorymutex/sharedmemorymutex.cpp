#include <TfTest/TfTest>
#include <QElapsedTimer>
#include "tsharedmemorykvs.h"
#include "tcommandlineinterface.h"
#include "tglobal.h"


const QString SHM_NAME = "tfcache.shm";
const QString OPTIONS = "MEMORY_SIZE=256M";


// static QByteArray randomString(int length)
// {
//     const auto ch = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-+/^=_[]@:;!#$%()~? \t\n";
//     QByteArray ret;
//     int max = (int)strlen(ch) - 1;
//     for (int i = 0; i < length; ++i) {
//         ret += ch[Tf::random(max)];
//     }
//     return ret;
// }



void insertItems()
{
    TSharedMemoryKvs smhash;
    QByteArray key, value, val;

    while (smhash.tableSize() < 10000 || smhash.loadFactor() < 0.5) {
        key = QUuid::createUuid().toByteArray();
        value = QCryptographicHash::hash(key, QCryptographicHash::Sha1).toHex();

        bool ok = smhash.set(key, value, 1000);
        assert(ok);
        if (!ok) {
            break;
        }
    }
}


void init()
{
    TSharedMemoryKvs::initialize(SHM_NAME, OPTIONS);
}


// Test setting/getting value
void test1(int d = 200000)
{
    TSharedMemoryKvs smhash;
    tDebug() << "test1 #0 smhash count:" << smhash.count() << "  table size:" << smhash.tableSize();

    // insert data
    insertItems();
    tDebug() << "test1 #1 smhash count:" << smhash.count() << "  table size:" << smhash.tableSize();

    QByteArray key, value, val, keyprev;
    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < d; i++) {
        key = QUuid::createUuid().toByteArray();
        value = QCryptographicHash::hash(key, QCryptographicHash::Sha1).toHex();

        bool ok = smhash.set(key, value, 1000);
        assert(ok);
        std::this_thread::yield();
        val = smhash.get(key);
        assert(val == value);
        std::this_thread::yield();

        if (i % 2) {
            ok = smhash.remove(key);
            assert(ok);
            val = smhash.get(key);
            assert(val == QByteArray());
            std::this_thread::yield();
        }

        // check previous value
        if (!keyprev.isEmpty()) {
            val = smhash.get(keyprev);
            assert(val == QCryptographicHash::hash(keyprev, QCryptographicHash::Sha1).toHex());
        }

        if (!(i % 2)) {
            if (!keyprev.isEmpty()) {
                // remove previous value
                ok = smhash.remove(keyprev);
                assert(ok);
                std::this_thread::yield();
                val = smhash.get(keyprev);
                assert(val == QByteArray());
            }
            keyprev = key;
        }
    }

    tDebug() << "test1 #2 smhash count:" << smhash.count() << "  table size:" << smhash.tableSize();
    tDebug() << "test1 ... done  "  << timer.elapsed() / 1000.0 << " sec elapsed\n";
}


// Test getting value
void test2(int d = 5000000)
{
    TSharedMemoryKvs smhash;
    tDebug() << "test2 #0 smhash count:" << smhash.count() << " table size:" << smhash.tableSize();

    // insert data
    insertItems();
    tDebug() << "test2 #1 smhash count:" << smhash.count() << " table size:" << smhash.tableSize();

    QByteArray key, value;
    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < d; i++) {
        key = QUuid::createUuid().toByteArray();
        value = smhash.get(key);
        assert(value == QByteArray());
    }

    tDebug() << "test2 ... done  "  << timer.elapsed() / 1000.0 << "sec elapsed\n";
}


static int command()
{
    Tf::setupAppLoggers(new TStdOutLogger);
    Tf::setAppLogLayout("{} %5P %m%n");
    Tf::setAppLogDateTimeFormat("hh:mm:ss");

    int num = 1;

    if (QCoreApplication::arguments().count() > 1) {
        num = QCoreApplication::arguments().at(1).toInt();
    } else {
        // Initialize
        init();
    }

    switch (num) {
    case 0:
        init();
        break;
    case 1:
        test1();
        break;
    case 2:
        test2();
        break;
    default:
        break;
    }

    // TSharedMemoryKvs smhash;
    // smhash.cleanup();
    return 0;
}


TF_CLI_MAIN(command)
