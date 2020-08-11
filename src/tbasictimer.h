#pragma once
#include <QBasicTimer>
#include <QObject>
#include <TGlobal>


class T_CORE_EXPORT TBasicTimer : public QObject, public QBasicTimer {
    Q_OBJECT
public:
    TBasicTimer(QObject *parent = 0);
    int interval() const { return _interval; }
    void setInterval(int interval) { _interval = interval; }
    void setReceiver(QObject *receiver) { _receiver = receiver; }

public slots:
    void start();
    void start(int msec);
    void stop() { QBasicTimer::stop(); }

private:
    int _interval {0};
    QObject *_receiver {nullptr};

    T_DISABLE_COPY(TBasicTimer)
    T_DISABLE_MOVE(TBasicTimer)
};

