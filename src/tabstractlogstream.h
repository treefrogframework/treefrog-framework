#pragma once
#include <QList>
#include <QObject>
#include <TLog>

class TLogger;


class TAbstractLogStream : public QObject {
    Q_OBJECT
public:
    TAbstractLogStream(const QList<TLogger *> &loggers, QObject *parent);
    virtual ~TAbstractLogStream() { }
    virtual void writeLog(const TLog &log) = 0;
    virtual void flush() = 0;

    bool isNonBufferingMode() const { return nonBuffering; }

public slots:
    virtual void setNonBufferingMode();

protected:
    enum LoggerType {
        All = 0,
        MultiProcessSafe,
        MultiProcessUnsafe,
    };

    bool loggerOpen(LoggerType type = All);
    void loggerClose(LoggerType type = All);
    void loggerWrite(const TLog &log);
    void loggerWrite(const QList<TLog> &logs);
    void loggerFlush();

private:
    QList<TLogger *> loggerList;
    bool nonBuffering;

    T_DISABLE_COPY(TAbstractLogStream)
    T_DISABLE_MOVE(TAbstractLogStream)
};

