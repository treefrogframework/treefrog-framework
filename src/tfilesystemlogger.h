#pragma once
#include "tsystemlogger.h"
#include <QString>
#include <QFile>
#include <QMutex>
#include <TGlobal>


class T_CORE_EXPORT TFileSystemLogger : public TSystemLogger {
public:
    TFileSystemLogger(const QString &name = QString());
    ~TFileSystemLogger();

    bool open() override;
    void close() override;
    bool isOpen() const override;
    int write(const char *data, int length) override;
    void flush() override;
    void setFileName(const QString &name);

private:
    QFile _logFile;
    QMutex _mutex;

    T_DISABLE_COPY(TFileSystemLogger)
    T_DISABLE_MOVE(TFileSystemLogger)
};
