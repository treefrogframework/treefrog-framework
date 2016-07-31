#include <QTest>
#include <QThread>
#include <QDebug>
#include <thazardpointer.h>
#include <thazardobject.h>
#include "stack.h"
#include <unistd.h>
#include <thread>


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
        for (;;) {
            //for (int i = 0; i < 100000; i++) {
            Box box;
            if (stackBox.pop(box)) {
                Q_ASSERT(box.a + box.b == 1000);
            } else {
                // std::this_thread::yield();
                //Tf::msleep(1);
            }
            //Tf::msleep(1);
        }
        //qDebug() << stackBox.count();
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
            //for (int i = 0; i < 100000; i++) {
            Box box;
            if (stackBox.peak(box)) {
                Q_ASSERT(box.a + box.b == 1000);

                box.a++;
                Tf::msleep(1);
                box.b--;
            } else {
                box.a = 1000;
                box.b = 0;
            }

            if (stackBox.count() < 100) {
                stackBox.push(box);
                std::this_thread::yield();
                //Tf::msleep(1);
            } else {
                //Tf::msleep(1);
            }
        }
        //qDebug() << stackBox.count();
    }
};


class TestHazardPointer : public QObject
{
    Q_OBJECT
    //public slots:
    void startPopThread();
    void startPushThread();

private slots:
    void push_pop();
};


void TestHazardPointer::push_pop()
{
    for (int i = 0; i < 1; i++) {
        startPopThread();
        startPushThread();
    }

    QEventLoop eventLoop;
    for (;;) {
        eventLoop.processEvents();
        Tf::msleep(1);
    }
}


void TestHazardPointer::startPopThread()
{
    auto *threada = new PushThread();
    connect(threada, SIGNAL(finished()), threada, SLOT(deleteLater()));
    //connect(threada, SIGNAL(finished()), this, SLOT(startPopThread()));
    threada->start();
    //threada->wait(10);
}


void TestHazardPointer::startPushThread()
{
    auto *threadb = new PopThread();
    connect(threadb, SIGNAL(finished()), threadb, SLOT(deleteLater()));
    //connect(threadb, SIGNAL(finished()), this, SLOT(startPushThread()));
    threadb->start();
    //threadb->wait(10);
}


//QTEST_APPLESS_MAIN(TestHazardPointer)
QTEST_MAIN(TestHazardPointer)
#include "main.moc"
