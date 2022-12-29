#pragma once
#include "tabstractlogstream.h"
#include <QBasicTimer>

class QSharedMemory;


class T_CORE_EXPORT TSharedMemoryLogStream : public TAbstractLogStream {
public:
    TSharedMemoryLogStream(const QList<TLogger *> loggers, int size = 4096, QObject *parent = 0);
    ~TSharedMemoryLogStream();

    void writeLog(const TLog &);
    void flush();
    QString errorString() const;
    void setNonBufferingMode();

protected:
    void loggerWriteLog(const QList<TLog> &logs);
    void clearBuffer();
    QList<TLog> smRead();
    bool smWrite(const QList<TLog> &logs);
    static int dataSizeOf(const QList<TLog> &logs);
    void timerEvent(QTimerEvent *event);

private:
    QSharedMemory *shareMem;
    QBasicTimer timer;

    T_DISABLE_COPY(TSharedMemoryLogStream)
    T_DISABLE_MOVE(TSharedMemoryLogStream)
};

