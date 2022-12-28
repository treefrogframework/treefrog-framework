#pragma once
#include <QFile>
#include <QMutex>
#include <TLogger>


class T_CORE_EXPORT TFileLogger : public TLogger {
public:
    TFileLogger();
    ~TFileLogger();

    QString key() const override { return "FileLogger"; }
    bool isMultiProcessSafe() const override { return false; }
    bool open() override;
    void close() override;
    bool isOpen() const override;
    void log(const QByteArray &msg) override;
    void flush() override;
    void setFileName(const QString &name);

private:
    QFile logFile;
    QMutex mutex;
};
