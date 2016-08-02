#include <QTest>
#include <QThread>
#include <QDebug>
#include <thazardpointer.h>
#include <thazardobject.h>
#include "stack.h"
#include <unistd.h>


class Box
{
public:
    int a { 0 };
    int b { 0 };

    Box() {}
    Box(const Box &box)
      : a(box.a), b(box.b) {}
    Box &operator=(const Box &box) {
        a = box.a;
        b = box.b;
        return *this;
    }
};
stack<Box> stackBox;


class PopThread : public QThread
{
    Q_OBJECT
public:
    PopThread() { }
protected:
    void run() {
        //for (;;) {
        for (int i = 0; i < 500000; i++) {
            Box box;
            if (stackBox.pop(box)) {
                Q_ASSERT(box.a + box.b == 1000);
            }
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
        //for (;;) {
        for (int i = 0; i < 500000; i++) {
            Box box;
            if (stackBox.peak(box)) {
                Q_ASSERT(box.a + box.b == 1000);

                box.a++;
                QThread::yieldCurrentThread();
                box.b--;
            } else {
                box.a = 1000;
                box.b = 0;
            }

            if (stackBox.count() < 100) {
                stackBox.push(box);
            }
        }
    }
};


class TestHazardPointer : public QObject
{
    Q_OBJECT
public slots:
    void startPopThread();
    void startPushThread();

private slots:
    void push_pop();
};


void TestHazardPointer::push_pop()
{
    // Starts threads
    for (int i = 0; i < 1000; i++) {
        startPopThread();
        startPopThread();
        startPushThread();
    }

    QElapsedTimer timer;
    timer.start();

    QEventLoop eventLoop;
    while (timer.elapsed() < 5000) {
        eventLoop.processEvents();
    }
    _exit(0);
}


void TestHazardPointer::startPopThread()
{
    auto *threada = new PopThread();
    connect(threada, SIGNAL(finished()), this, SLOT(startPopThread()));
    connect(threada, SIGNAL(finished()), threada, SLOT(deleteLater()));
    threada->start();
}


void TestHazardPointer::startPushThread()
{
    auto *threadb = new PushThread();
    connect(threadb, SIGNAL(finished()), this, SLOT(startPushThread()));
    connect(threadb, SIGNAL(finished()), threadb, SLOT(deleteLater()));
    threadb->start();
}


QTEST_MAIN(TestHazardPointer)
#include "main.moc"
