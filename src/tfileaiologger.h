#ifndef TFILEAIOLOGGER_H
#define TFILEAIOLOGGER_H

#include <QMutex>
#include <QString>
#include <QList>
#include <TLogger>

struct aiocb;


class T_CORE_EXPORT TFileAioLogger : public TLogger
{
public:
    TFileAioLogger();
    ~TFileAioLogger();

    QString key() const { return "FileAioLogger"; }
    bool isMultiProcessSafe() const { return true; }
    bool open();
    void close();
    bool isOpen() const;
    void log(const TLog &log);
    void log(const QByteArray &msg);
    void flush();
    void setFileName(const QString &name);

private:
    void clearSyncBuffer();

    mutable QMutex mutex;
    QString fileName;
    int fileDescriptor;
    QList<struct aiocb *> syncBuffer;
};

#endif // TFILEAIOLOGGER_H
