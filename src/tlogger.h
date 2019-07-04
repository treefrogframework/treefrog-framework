#ifndef TLOGGER_H
#define TLOGGER_H

#include <QString>
#include <QVariant>
#include <TGlobal>
#include <TLog>

class TLog;
class QTextCodec;


class T_CORE_EXPORT TLogger
{
public:
    TLogger();
    virtual ~TLogger() { }
    virtual QString key() const = 0;
    virtual bool isMultiProcessSafe() const = 0;
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual void log(const TLog &log) = 0; // thread safe log output
    virtual void log(const QByteArray &) { } // thread safe log output
    virtual void flush() { }
    virtual QByteArray logToByteArray(const TLog &log) const;

    void readSettings();
    const QByteArray &layout() const { return _layout; }
    const QByteArray &dateTimeFormat() const { return _dateTimeFormat; }
    Tf::LogPriority threshold() const { return _threshold; }
    const QString &target() const { return _target; }
    QVariant settingsValue(const QString &key, const QVariant &defaultValue = QVariant()) const;

    static QByteArray logToByteArray(const TLog &log, const QByteArray &layout, const QByteArray &dateTimeFormat, QTextCodec *codec = 0);
    static QByteArray priorityToString(Tf::LogPriority priority);

protected:
    QByteArray _layout;
    QByteArray _dateTimeFormat;
    Tf::LogPriority _threshold {Tf::TraceLevel};
    QString  _target;
    QTextCodec *_codec {nullptr};
};

#endif // TLOGGER_H
