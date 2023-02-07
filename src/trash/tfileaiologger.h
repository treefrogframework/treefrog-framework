#pragma once
#include <QString>
#include <TLogger>

class TFileAioWriter;


class TFileAioLogger : public TLogger {
public:
    TFileAioLogger();
    ~TFileAioLogger();

    QString key() const override { return "FileAioLogger"; }
    bool isMultiProcessSafe() const override { return true; }
    bool open() override;
    void close() override;
    bool isOpen() const override;
    void log(const QByteArray &msg) override;
    void log(const TLog &tlog) override { TLogger::log(tlog); }
    void flush() override;
    void setFileName(const QString &name);

private:
    TFileAioWriter *writer {nullptr};

    T_DISABLE_COPY(TFileAioLogger)
    T_DISABLE_MOVE(TFileAioLogger)
};

