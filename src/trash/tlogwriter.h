#ifndef TLOGWRITER_H
#define TLOGWRITER_H

#include <QByteArray>
#include <TGlobal>


class T_CORE_EXPORT TLogWriter
{
public:
    static void write(const char *msg);
};


/**
 * TAbstractLogWriter class
 */
class TAbstractLogWriter
{
public:
    virtual ~TAbstractLogWriter() { }
    virtual void writeLog(const char *msg) = 0;
};

#endif // TLOGWRITER_H
