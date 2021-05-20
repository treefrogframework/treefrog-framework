/* Copyright (c) 2010-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QDir>
#include <QFileInfo>
#include <QTextCodec>
#include <TLogger>
#include <TSystemGlobal>
#include <TWebApplication>

constexpr auto DEFAULT_TEXT_ENCODING = "DefaultTextEncoding";


class PriorityHash : public QMap<Tf::LogPriority, QByteArray> {
public:
    PriorityHash() :
        QMap<Tf::LogPriority, QByteArray>()
    {
        insert(Tf::FatalLevel, "FATAL");
        insert(Tf::ErrorLevel, "ERROR");
        insert(Tf::WarnLevel, "WARN");
        insert(Tf::InfoLevel, "INFO");
        insert(Tf::DebugLevel, "DEBUG");
        insert(Tf::TraceLevel, "TRACE");
    }
};
Q_GLOBAL_STATIC(PriorityHash, priorityHash)


/*!
  \class TLogger
  \brief The TLogger class provides an abstract base of logging functionality.
*/

/*!
  Constructor.
*/
TLogger::TLogger()
{
}

/*!
  Returns the value for logger setting \a key. If the setting doesn't exist,
  returns \a defaultValue.
*/
QVariant TLogger::settingsValue(const QString &k, const QVariant &defaultValue) const
{
    const auto &settings = Tf::app()->loggerSettings();
    //tSystemDebug("settingsValue: %s", qUtf8Printable(key() + "." + k));
    return settings.value(key() + "." + k, defaultValue);
}

/*!
  Converts the log \a log to its textual representation and returns
  a QByteArray containing the data.
*/
QByteArray TLogger::logToByteArray(const TLog &log) const
{
    return logToByteArray(log, layout(), dateTimeFormat(), codec());
}

/*!
  Converts the log \a log to its textual representation and returns
  a QByteArray containing the data.
*/
QByteArray TLogger::logToByteArray(const TLog &log, const QByteArray &layout, const QByteArray &dateTimeFormat, QTextCodec *codec)
{
    QByteArray message;
    int pos = 0;
    QByteArray dig;
    message.reserve(layout.length() + log.message.length() + 100);

    while (pos < layout.length()) {
        char c = layout.at(pos++);
        if (c != '%') {
            message.append(c);
            continue;
        }

        dig.resize(0);
        for (;;) {
            if (pos >= layout.length()) {
                message.append('%').append(dig);
                break;
            }

            c = layout.at(pos++);
            if (c >= '0' && c <= '9') {
                dig += c;
                continue;
            }

            switch (c) {
            case 'd':  // %d : timestamp
                if (!dateTimeFormat.isEmpty()) {
                    message.append(log.timestamp.toString(dateTimeFormat).toLatin1());
                } else {
                    message.append(log.timestamp.toString(Qt::ISODate).toLatin1());
                }
                break;

            case 'p':
            case 'P': {  // %p or %P : priority
                QByteArray pri = priorityToString((Tf::LogPriority)log.priority);
                if (c == 'p') {
                    pri = pri.toLower();
                }
                if (!pri.isEmpty()) {
                    message.append(pri);
                    int d = dig.toInt() - pri.length();
                    if (d > 0) {
                        message.append(QByteArray(d, ' '));
                    }
                }
                break;
            }

            case 't':
            case 'T': {  // %t or %T : thread ID (dec or hex)
                const QChar fillChar = (dig.length() > 0 && dig[0] == '0') ? QLatin1Char('0') : QLatin1Char(' ');
                message.append(QString("%1").arg((qulonglong)log.threadId, dig.toInt(), ((c == 't') ? 10 : 16), fillChar).toLatin1());
                break;
            }

            case 'i':
            case 'I': {  // %i or %I : PID (dec or hex)
                const QChar fillChar = (dig.length() > 0 && dig[0] == '0') ? QLatin1Char('0') : QLatin1Char(' ');
                message.append(QString("%1").arg(log.pid, dig.toInt(), ((c == 'i') ? 10 : 16), fillChar).toLatin1());
                break;
            }

            case 'n':  // %n : newline
                message.append('\n');
                break;

            case 'm':  // %m : message
                message.append(log.message);
                break;

            case '%':
                message.append('%').append(dig);
                dig.resize(0);
                continue;
                break;

            default:
                message.append('%').append(dig).append(c);
                break;
            }
            break;
        }
    }

    return (codec) ? codec->fromUnicode(QString::fromLocal8Bit(message.data(), message.length())) : message;
}

/*!
  Returns a QByteArray containing the priority \a priority.
*/
QByteArray TLogger::priorityToString(Tf::LogPriority priority)
{
    return priorityHash()->value(priority);
}

/*!
  Returns a pointer to QTextCodec of the default text encoding.
 */
QTextCodec *TLogger::codec() const
{
    if (!_codec) {
        // Sets the codec
        auto &settings = Tf::app()->loggerSettings();
        QByteArray codecName = settings.value(DEFAULT_TEXT_ENCODING).toByteArray().trimmed();
        if (!codecName.isEmpty()) {
            QTextCodec *c = QTextCodec::codecForName(codecName);
            if (c) {
                if (c->name() != QTextCodec::codecForLocale()->name()) {
                    _codec = c;
                }
                //tSystemDebug("set log text codec: %s", c->name().data());
            } else {
                tSystemError("log text codec matching the name could be not found: %s", codecName.data());
            }
        }
    }
    return _codec;
}

/*!
  Returns a reference to the value for the setting layout.
*/
const QByteArray &TLogger::layout() const
{
    if (_layout.isEmpty()) {
        _layout = settingsValue("Layout", "%m%n").toByteArray();
    }
    return _layout;
}

/*!
  Returns a reference to the value for the setting datetime format.
*/
const QByteArray &TLogger::dateTimeFormat() const
{
    if (_dateTimeFormat.isEmpty()) {
        _dateTimeFormat = settingsValue("DateTimeFormat").toByteArray();
    }
    return _dateTimeFormat;
}

/*!
  Returns the value for the setting priority threshold.
*/
Tf::LogPriority TLogger::threshold() const
{
    if ((int)_threshold < 0) {
        QByteArray pri = settingsValue("Threshold", "trace").toByteArray().toUpper().trimmed();
        _threshold = priorityHash()->key(pri, Tf::TraceLevel);
    }
    return _threshold;
}

/*!
  Returns a reference to the value for the setting target device.
*/
const QString &TLogger::target() const
{
    if (_target.isEmpty()) {
        QString logtarget = settingsValue("Target", "log/app.log").toString().trimmed();
        if (!logtarget.isEmpty()) {
            QFileInfo fi(logtarget);
            _target = (fi.isAbsolute()) ? fi.absoluteFilePath() : Tf::app()->webRootPath() + fi.filePath();

            QDir dir = QFileInfo(_target).dir();
            if (!dir.exists()) {
                // Created a directory
                dir.mkpath(".");
            }
        } else {
            tSystemWarn("Empty file name for application log.");
        }
    }
    return _target;
}


/*!
  \fn virtual QString TLogger::key() const
  Returns a key that this logger plugin supports.
*/

/*!
  \fn virtual bool TLogger::isMultiProcessSafe() const
  Returns true if the implementation is guaranteed to be free of race
  conditions when accessed by multiple processes simultaneously; otherwise
  returns false.
*/

/*!
  \fn virtual bool TLogger::open()
  Opens the device for logging. Returns true if successful; otherwise returns
  false. This function should be called from any reimplementations of open().
*/

/*!
  \fn virtual void TLogger::close()
  Closes the device. This function should be called from any reimplementations
  of close().
*/

/*!
  \fn virtual bool TLogger::isOpen() const
  Returns true if the device is open; otherwise returns false.
  This function should be called from any reimplementations of isOpen().
*/

/*!
  \fn virtual void TLogger::log(const TLog &log)
  Writes the log \a log to the device.
  This function should be called from any reimplementations of log().
*/

/*!
  \fn virtual void TLogger::flush()
  Flushes any buffered data to the device.
  This function should be called from any reimplementations of flush().
*/
