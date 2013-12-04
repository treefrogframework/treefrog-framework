#ifndef TFILEAIOLOGGER_H
#define TFILEAIOLOGGER_H

#include <QString>
#include <TLogger>

class TFileAioWriter;


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
    TFileAioWriter *writer;

    Q_DISABLE_COPY(TFileAioLogger)
};

#endif // TFILEAIOLOGGER_H
