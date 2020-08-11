#pragma once
#include <QString>
#include <QVariant>
#include <TGlobal>
#include <TLog>

class TLog;
class QTextCodec;


class T_CORE_EXPORT TLogger {
public:
    TLogger();
    virtual ~TLogger() { }
    virtual QString key() const = 0;
    virtual bool isMultiProcessSafe() const = 0;
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual void log(const TLog &log) = 0;  // thread safe log output
    virtual void log(const QByteArray &) { }  // thread safe log output
    virtual void flush() { }
    virtual QByteArray logToByteArray(const TLog &log) const;

    const QByteArray &layout() const;
    const QByteArray &dateTimeFormat() const;
    Tf::LogPriority threshold() const;
    const QString &target() const;

    static QByteArray logToByteArray(const TLog &log, const QByteArray &layout, const QByteArray &dateTimeFormat, QTextCodec *codec = 0);
    static QByteArray priorityToString(Tf::LogPriority priority);

protected:
    QTextCodec *codec() const;
    QVariant settingsValue(const QString &key, const QVariant &defaultValue = QVariant()) const;

private:
    mutable QByteArray _layout;
    mutable QByteArray _dateTimeFormat;
    mutable Tf::LogPriority _threshold {(Tf::LogPriority)-1};
    mutable QString _target;
    mutable QTextCodec *_codec {nullptr};
};

