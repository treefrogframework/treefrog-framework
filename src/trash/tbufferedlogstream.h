#ifndef TBUFFEREDLOGSTREAM_H
#define TBUFFEREDLOGSTREAM_H

#include <QObject>
#include <QMutex>
#include <QList>
#include <QBasicTimer>
#include "tabstractlogstream.h"


class TBufferedLogStream : public QObject, public TAbstractLogStream
{
    Q_OBJECT
public:
    TBufferedLogStream(const QList<TLogger *> loggers, QObject *parent = 0);
    ~TBufferedLogStream();

    void writeLog(const TLog &);
    void flush();

public slots:
    void setNonBufferingMode();

protected:
    void timerEvent(QTimerEvent *event);

private:
    QList<TLog> logList;
    QMutex mutex;
    QBasicTimer timer;
    bool nonBuffering;
};

#endif // TBUFFEREDLOGSTREAM_H

