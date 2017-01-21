/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QFileInfo>
#include <QDir>
#include <QTextCodec>
#include <TLogger>
#include <TWebApplication>
#include <TSystemGlobal>

#define DEFAULT_TEXT_ENCODING "DefaultTextEncoding"


class PriorityHash : public QMap<Tf::LogPriority, QByteArray>
{
public:
    PriorityHash() : QMap<Tf::LogPriority, QByteArray>()
    {
        insert(Tf::FatalLevel, "FATAL");
        insert(Tf::ErrorLevel, "ERROR");
        insert(Tf::WarnLevel,  "WARN");
        insert(Tf::InfoLevel,  "INFO");
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
TLogger::TLogger() : threshold_(Tf::TraceLevel), codec_(nullptr)
{ }

/*!
  Returns the value for logger setting \a key. If the setting doesn't exist,
  returns \a defaultValue.
*/
QVariant TLogger::settingsValue(const QString &k, const QVariant &defaultValue) const
{
    const QSettings &settings = Tf::app()->loggerSettings();
    //tSystemDebug("settingsValue: %s", qPrintable(key() + "." + k));
    return settings.value(key() + "." + k, defaultValue);
}

/*!
  Converts the log \a log to its textual representation and returns
  a QByteArray containing the data.
*/
QByteArray TLogger::logToByteArray(const TLog &log) const
{
    return logToByteArray(log, layout(), dateTimeFormat(), codec_);
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

        dig.clear();
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
                break; }

            case 't':
            case 'T': { // %t or %T : thread ID (dec or hex)
                const QChar fillChar = (dig.length() > 0 && dig[0] == '0') ? QLatin1Char('0') : QLatin1Char(' ');
                message.append(QString("%1").arg((qulonglong)log.threadId, dig.toInt(), ((c == 't') ? 10 : 16), fillChar).toLatin1());
                break; }

            case 'i':
            case 'I': {  // %i or %I : PID (dec or hex)
                const QChar fillChar = (dig.length() > 0 && dig[0] == '0') ? QLatin1Char('0') : QLatin1Char(' ');
                message.append(QString("%1").arg(log.pid, dig.toInt(), ((c == 'i') ? 10 : 16), fillChar).toLatin1());
                break; }

            case 'n':  // %n : newline
                message.append('\n');
                break;

            case 'm':  // %m : message
                message.append(log.message);
                break;

            case '%':
                message.append('%').append(dig);
                dig.clear();
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
  Reads the settings stored in the file \a logger.ini.
*/
void TLogger::readSettings()
{
    // Sets the codec
    QSettings &settings = Tf::app()->loggerSettings();
    QByteArray codecName = settings.value(DEFAULT_TEXT_ENCODING).toByteArray().trimmed();
    if (!codecName.isEmpty()) {
        QTextCodec *c = QTextCodec::codecForName(codecName);
        if (c) {
            if (c->name() != QTextCodec::codecForLocale()->name()) {
                codec_ = c;
            }
            //tSystemDebug("set log text codec: %s", c->name().data());
        } else {
            tSystemError("log text codec matching the name could be not found: %s", codecName.data());
        }
    }

    layout_ = settingsValue("Layout", "%m%n").toByteArray();
    dateTimeFormat_ = settingsValue("DateTimeFormat").toByteArray();

    QByteArray pri = settingsValue("Threshold", "trace").toByteArray().toUpper().trimmed();
    threshold_ = priorityHash()->key(pri, Tf::TraceLevel);

    QFileInfo fi(settingsValue("Target", "log/app.log").toString());
    target_ = (fi.isAbsolute()) ? fi.absoluteFilePath() : Tf::app()->webRootPath() + fi.filePath();

    QDir dir = QFileInfo(target_).dir();
    if (!dir.exists()) {
        // Created a directory
        dir.mkpath(".");
    }
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

/*!
  \fn const QByteArray &TLogger::layout() const
  Returns a reference to the value for the setting layout.
*/

/*!
  \fn const QByteArray &TLogger::dateTimeFormat() const
  Returns a reference to the value for the setting datetime format.
*/

/*!
  \fn Priority TLogger::threshold() const
  Returns the value for the setting priority threshold.
*/

/*!
  \fn const QString &TLogger::target() const
  Returns a reference to the value for the setting target device.
*/
