#pragma once
#include "tsystemlogger.h"


class T_CORE_EXPORT TStdOutSystemLogger : public TSystemLogger {
public:
    virtual ~TStdOutSystemLogger() {}
    bool open() override { return true; }
    void close() override {}
    bool isOpen() const override { return true; }
    int write(const char *data, int length) override;
    void flush() override;
};
