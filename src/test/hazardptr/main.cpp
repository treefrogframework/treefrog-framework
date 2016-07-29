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
        for (int i = 0; i < 1000; i++) {
            Box box;
            stackBox.pop(box);
        }
        qDebug() << stackBox.count();
    }
};

class PushThread : public QThread
{
    Q_OBJECT
public:
    PushThread() { }
protected:
    void run() {
        for (int i = 0; i < 1000; i++) {
            Box box;
            if (stackBox.peak(box)) {
                box.a++;
                box.b--;
            } else {
                box.a = 1000;
                box.b = 0;
            }
            stackBox.push(box);
        }
        qDebug() << stackBox.count();
    }
};


class TestHazardPointer : public QObject
{
    Q_OBJECT
private slots:
    void push_pop();
};


void TestHazardPointer::push_pop()
{
    for (int i = 0; i < 1000; i++) {
        auto *threada = new PushThread();

        connect(threada, SIGNAL(finished()), threada, SLOT(deleteLater()));

        threada->start();
    }
    for (int i = 0; i < 1000; i++) {
        auto *threadb = new PopThread();

        connect(threadb, SIGNAL(finished()), threadb, SLOT(deleteLater()));

        threadb->start();
    }
    sleep(20);
}

QTEST_APPLESS_MAIN(TestHazardPointer)
#include "main.moc"
