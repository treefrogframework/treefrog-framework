#ifndef TBASICTIMER_H
#define TBASICTIMER_H

#include <QObject>
#include <QBasicTimer>
#include <TGlobal>


class T_CORE_EXPORT TBasicTimer : public QObject, public QBasicTimer
{
    Q_OBJECT
public:
    TBasicTimer(QObject *parent = 0);
    int interval() const { return interval_; }
    void setInterval(int interval) { interval_ = interval; }
    void setReceiver(QObject *receiver) { receiver_ = receiver; }

public slots:
    void start();
    void start(int msec);
    void stop() { QBasicTimer::stop(); }

private:
    int interval_;
    QObject *receiver_;

    T_DISABLE_COPY(TBasicTimer)
    T_DISABLE_MOVE(TBasicTimer)
};

#endif // TBASICTIMER_H
