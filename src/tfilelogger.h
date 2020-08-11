#pragma once
#include <QFile>
#include <QMutex>
#include <TLogger>


class T_CORE_EXPORT TFileLogger : public TLogger {
public:
    TFileLogger();
    ~TFileLogger();

    QString key() const { return "FileLogger"; }
    bool isMultiProcessSafe() const { return false; }
    bool open();
    void close();
    bool isOpen() const;
    void log(const TLog &log);
    void log(const QByteArray &msg);
    void flush();
    void setFileName(const QString &name);

private:
    QFile logFile;
    QMutex mutex;
};

