#ifndef TSTANDARDOUTPUTWRITER_H
#define TSTANDARDOUTPUTWRITER_H

#include <QByteArray>
#include <TLogWriter>


class TStandardOutputWriter : public TAbstractLogWriter
{
public:
    TStandardOutputWriter() { }
    virtual ~TStandardOutputWriter() { }
    
    void writeLog(const char *msg);
};

#endif // TSTANDARDOUTPUTWRITER_H
