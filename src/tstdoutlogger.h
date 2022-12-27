#pragma once
#include <TLogger>


class T_CORE_EXPORT TStdOutLogger : public TLogger {
public:
    TStdOutLogger();

    QString key() const override { return "StdOutLogger"; }
    bool isMultiProcessSafe() const override { return true; }
    bool open() override { return true; }
    void close() override {}
    bool isOpen() const override { return true; }
    void log(const QByteArray &) override;
    void flush() override;
};
