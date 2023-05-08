#include <QTest>
#include <QThread>
#include <atomic>
#include <iostream>
#include <thread>
#include "tstack.h"
#ifndef Q_CC_MSVC
# include <unistd.h>
#endif
#include <TfTest/TfTest>
#include <glog/logging.h>


static std::atomic<int64_t> counter {0};  // Box counter

#if defined(Q_OS_UNIX) || !defined(QT_NO_DEBUG)
void writeFailure(const char *data, size_t size)
{
    std::cout << QByteArray(data, size).data() << std::endl;
}
#endif


struct Box
{
    int a {0};
    int b {0};

    Box() { counter++; }
    Box(const Box &box)
        : a(box.a), b(box.b) { counter++; }
    ~Box() { counter--; }
    Box &operator=(const Box &box) {
        a = box.a;
        b = box.b;
        return *this;
    }
};
TStack<Box> stackBox;


class PopThread : public QThread
{
    Q_OBJECT
public:
    PopThread() { }
protected:
    void run() {
        for (;;) {
            Box box;
            if (stackBox.pop(box)) {
                Q_ASSERT(box.a + box.b == 1000);
            }
            //Tf::msleep(1);
            std::this_thread::yield();
        }
    }
};


class PushThread : public QThread
{
    Q_OBJECT
public:
    PushThread() {}
protected:
    void run() {
        for (;;) {
            Box box;
            if (stackBox.top(box)) {
                Q_ASSERT(box.a + box.b == 1000);
                box.a++;
                box.b--;
            } else {
                box.a = 1000;
                box.b = 0;
            }

            if (stackBox.count() < 500) {
                stackBox.push(box);
            }
            //Tf::msleep(1);
            std::this_thread::yield();
        }
    }
};


class StackStarter : public QObject
{
    Q_OBJECT
public slots:
    void startPopThread();
    void startPushThread();
};


class TestStack : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void push_pop();
};


void TestStack::initTestCase()
{
#if defined(Q_OS_UNIX) || !defined(QT_NO_DEBUG)
    // Setup signal handlers for SIGSEGV, SIGILL, SIGFPE, SIGABRT and SIGBUS
    google::InstallFailureWriter(writeFailure);
    google::InstallFailureSignalHandler();
#endif
}


void TestStack::cleanupTestCase()
{
    _exit(0);
}


void TestStack::push_pop()
{
#ifdef Q_OS_UNIX
    const int NUM = 128;
#else
    const int NUM = std::max((int)std::thread::hardware_concurrency(), 2);
#endif

    // Starts threads
    StackStarter starter;
    for (int i = 0; i < NUM; i++) {
        starter.startPopThread();
        starter.startPushThread();
    }

    QElapsedTimer timer;
    timer.start();

    QEventLoop eventLoop;
    while (timer.elapsed() < 10000) {
        eventLoop.processEvents();
    }

    std::cout << "counter=" << counter.load() << std::endl;
    std::cout << "stack count=" << stackBox.count() << std::endl;
}


void StackStarter::startPopThread()
{
    auto *threada = new PopThread();
    connect(threada, SIGNAL(finished()), this, SLOT(startPopThread()));
    connect(threada, SIGNAL(finished()), threada, SLOT(deleteLater()));
    threada->start();
}


void StackStarter::startPushThread()
{
    auto *threadb = new PushThread();
    connect(threadb, SIGNAL(finished()), this, SLOT(startPushThread()));
    connect(threadb, SIGNAL(finished()), threadb, SLOT(deleteLater()));
    threadb->start();
}


TF_TEST_SQLLESS_MAIN(TestStack)
#include "main.moc"
