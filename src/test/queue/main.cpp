#include <QTest>
#include <QThread>
#include <atomic>
#include <iostream>
#include <thread>
#include <TfTest/TfTest>
#ifndef Q_CC_MSVC
# include <unistd.h>
#endif
#include "tqueue.h"
#include <glog/logging.h>


TQueue<uint64_t> intQueue;
std::atomic<uint64_t> generator {0};

#if defined(Q_OS_UNIX) || !defined(QT_NO_DEBUG)
void writeFailure(const char *data, size_t size)
{
    std::cout << QByteArray(data, size).data() << std::endl;
}
#endif

class PopThread : public QThread
{
    Q_OBJECT
public:
    PopThread() { }
protected:
    void run() override {
        try {
            uint64_t lastNum = 0;
            for (;;) {
                uint64_t num;
                if (intQueue.dequeue(num)) {
                    //std::cout << "pop:" << intQueue.count() << std::endl;
                    QVERIFY(num == lastNum);
                    lastNum++;
                    std::this_thread::yield();
                }
            }
        } catch (...) {}
        std::cout << "PopThread ...done. queue cnt:" << intQueue.count() << std::endl;
    }
};

class PushThread : public QThread
{
    Q_OBJECT
public:
    PushThread() { }
protected:
    void run() override {
        try {
            for (;;) {
                if (intQueue.count() < 1000) {
                    intQueue.enqueue(generator.fetch_add(1));
                    //std::cout << "queue cnt:" << intQueue.count() << std::endl;
                } else {
                    std::this_thread::yield();
                }
            }
        } catch (...) {}
        std::cout << "PushThread ...done. queue cnt:" << intQueue.count() << std::endl;
    }
};


class ThreadStarter : public QObject
{
    Q_OBJECT
public slots:
    void startPopThread();
    void startPushThread();
};


class TestQueue : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void queue();
};


void TestQueue::initTestCase()
{
#if defined(Q_OS_UNIX) || !defined(QT_NO_DEBUG)
    // Setup signal handlers for SIGSEGV, SIGILL, SIGFPE, SIGABRT and SIGBUS
    google::InstallFailureWriter(writeFailure);
    google::InstallFailureSignalHandler();
#endif
}


void TestQueue::cleanupTestCase()
{
    _exit(0);
}


void TestQueue::queue()
{
    // Starts threads
    ThreadStarter starter;
    starter.startPopThread();
    starter.startPushThread();

    QElapsedTimer timer;
    timer.start();

    QEventLoop eventLoop;
    while (timer.elapsed() < 10000) {
        eventLoop.processEvents();
    }

    std::cout << "queue count=" << intQueue.count() << std::endl;
}


void ThreadStarter::startPopThread()
{
    auto *threada = new PopThread();
    //connect(threada, SIGNAL(finished()), this, SLOT(startPopThread()));
    connect(threada, SIGNAL(finished()), threada, SLOT(deleteLater()));
    threada->start();
}


void ThreadStarter::startPushThread()
{
    auto *threadb = new PushThread();
    //connect(threadb, SIGNAL(finished()), this, SLOT(startPushThread()));
    connect(threadb, SIGNAL(finished()), threadb, SLOT(deleteLater()));
    threadb->start();
}


TF_TEST_SQLLESS_MAIN(TestQueue)
#include "main.moc"
