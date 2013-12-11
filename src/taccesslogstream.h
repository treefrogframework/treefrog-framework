#ifndef TACCESSLOGSTREAM_H
#define TACCESSLOGSTREAM_H

#include <QString>
#include <QByteArray>
#include <TGlobal>

class TLogger;


class T_CORE_EXPORT TAccessLogStream
{
public:
    TAccessLogStream(const QString &fileName);
    ~TAccessLogStream();
    void writeLog(const QByteArray &log);
    void flush();

private:
    TLogger *logger;

    // Disable
    TAccessLogStream();
    Q_DISABLE_COPY(TAccessLogStream)
};

#endif // TACCESSLOGSTREAM_H
