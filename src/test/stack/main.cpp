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

static std::atomic<qint64> counter {0};

struct Box
{
    int a {0};
    int b {0};

    Box() { counter++; }
    Box(const Box &box)
        : a(box.a), b(box.b) { counter++; }
    Box &operator=(const Box &box) {
        counter++;
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
        //for (int i = 0; i < 500000; i++) {
            Box box;
            if (stackBox.pop(box)) {
                Q_ASSERT(box.a + box.b == 1000);
            }
            std::this_thread::yield();
        }
    }
};


class PushThread : public QThread
{
    Q_OBJECT
public:
    PushThread() { }
protected:
    void run() {
        for (;;) {
        //for (int i = 0; i < 500000; i++) {
            Box box;
            if (stackBox.top(box)) {
                Q_ASSERT(box.a + box.b == 1000);

                box.a++;
                std::this_thread::yield();
                box.b--;
            } else {
                box.a = 1000;
                std::this_thread::yield();
                box.b = 0;
            }

            if (stackBox.count() < 100) {
                stackBox.push(box);
            }
        }
    }
};


class TestStack : public QObject
{
    Q_OBJECT
public slots:
    void startPopThread();
    void startPushThread();

private slots:
    void push_pop();
};


void TestStack::push_pop()
{
#ifdef Q_OS_LINUX
    const int NUM = 128;
#else
    const int NUM = 32;
#endif

    // Starts threads
    for (int i = 0; i < NUM; i++) {
        startPopThread();
        startPopThread();
        startPushThread();
    }

    QElapsedTimer timer;
    timer.start();

    QEventLoop eventLoop;
    while (timer.elapsed() < 10000) {
        eventLoop.processEvents();
    }

    std::cout << "counter=" << counter.load() << std::endl;
    std::cout << "stack count=" << stackBox.count() << std::endl;
    _exit(0);
}


void TestStack::startPopThread()
{
    auto *threada = new PopThread();
    connect(threada, SIGNAL(finished()), this, SLOT(startPopThread()));
    connect(threada, SIGNAL(finished()), threada, SLOT(deleteLater()));
    threada->start();
}


void TestStack::startPushThread()
{
    auto *threadb = new PushThread();
    connect(threadb, SIGNAL(finished()), this, SLOT(startPushThread()));
    connect(threadb, SIGNAL(finished()), threadb, SLOT(deleteLater()));
    threadb->start();
}


TF_TEST_SQLLESS_MAIN(TestStack)
#include "main.moc"
