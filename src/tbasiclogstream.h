#ifndef TBASICLOGSTREAM_H
#define TBASICLOGSTREAM_H

#include <QMutex>
#include <QBasicTimer>
#include "tabstractlogstream.h"


class T_CORE_EXPORT TBasicLogStream : public TAbstractLogStream
{
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

    Q_DISABLE_COPY(TBasicLogStream)
};

#endif // TBASICLOGSTREAM_H
