#pragma once
#include "tabstractlogstream.h"
#include <QBasicTimer>
#include <QMutex>


class T_CORE_EXPORT TBasicLogStream : public TAbstractLogStream {
public:
    TBasicLogStream(const QList<TLogger *> loggers, QObject *parent = 0);
    ~TBasicLogStream();

    void writeLog(const TLog &log);
    void flush();
    void setNonBufferingMode();

protected:
    void timerEvent(QTimerEvent *event);

private:
    QMutex mutex;
    QBasicTimer timer;

    T_DISABLE_COPY(TBasicLogStream)
    T_DISABLE_MOVE(TBasicLogStream)
};

