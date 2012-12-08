#ifndef TLOGFILEWRITER_H
#define TLOGFILEWRITER_H

#include <QString>
#include <QByteArray>
#include <QMutex>
#include <QFile>
#include <TLogWriter>


class TLogFileWriter : public TAbstractLogWriter
{
public:
    TLogFileWriter(const QString &logFileName);
    virtual ~TLogFileWriter();
    void writeLog(const char *msg);

private:
    QFile logFile;
    QMutex mutex;
};

#endif // TLOGFILEWRITER_H
