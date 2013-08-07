#ifndef TACCESSLOGSTREAM_H
#define TACCESSLOGSTREAM_H

class QSystemSemaphore;
class TFileLogger;


class TAccessLogStream
{
public:
    TAccessLogStream(const QString &fileName);
    ~TAccessLogStream();
    void writeLog(const QByteArray &log);

private:
    TFileLogger *logger;
    QSystemSemaphore *semaphore;

    // Disabled
    TAccessLogStream();
    TAccessLogStream(const TAccessLogStream &);
    TAccessLogStream &operator=(const TAccessLogStream &);
};

#endif // TACCESSLOGSTREAM_H
