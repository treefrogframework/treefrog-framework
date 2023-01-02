#pragma once
#include "tsystemlogger.h"


class T_CORE_EXPORT TStdErrSystemLogger : public TSystemLogger {
public:
    virtual ~TStdErrSystemLogger() {}
    bool open() override { return true; }
    void close() override {}
    bool isOpen() const override { return true; }
    int write(const char *data, int length) override;
    void flush() override;
};
