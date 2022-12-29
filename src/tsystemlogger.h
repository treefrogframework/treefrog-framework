#pragma once
#include <TGlobal>


class T_CORE_EXPORT TSystemLogger {
public:
    virtual ~TSystemLogger() {}
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual int write(const char *data, int length) = 0;
    virtual void flush() = 0;
};
