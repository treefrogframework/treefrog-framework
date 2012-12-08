/* Copyright (c) 2010-2012, AOYAMA Kazuharu
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

typedef QHash<TLogger::Priority, QByteArray> PriorityHash;
Q_GLOBAL_STATIC_WITH_INITIALIZER(PriorityHash, priorityHash,
{
    x->insert(TLogger::Fatal, "FATAL");
    x->insert(TLogger::Error, "ERROR");
    x->insert(TLogger::Warn,  "WARN");
    x->insert(TLogger::Info,  "INFO");
    x->insert(TLogger::Debug, "DEBUG");
    x->insert(TLogger::Trace, "TRACE");
});

/*!
  \class TLogger
  \brief The TLogger class provides an abstract base of logging functionality.
*/

TLogger::TLogger() : threshold_(Trace), codec_(0)
{ }


QVariant TLogger::settingsValue(const QString &k, const QVariant &defaultValue) const
{
    QSettings &settings = Tf::app()->loggerSettings();
    //tSystemDebug("settingsValue: %s", qPrintable(key() + "." + k));
    return settings.value(key() + "." + k, defaultValue);
}


QByteArray TLogger::logToByteArray(const TLog &log) const
{
    return logToByteArray(log, layout(), dateTimeFormat(), codec_);
}


QByteArray TLogger::logToByteArray(const TLog &log, const QByteArray &layout, const QByteArray &dateTimeFormat, QTextCodec *codec)
{
    QByteArray message;
    message.reserve(layout.length() + log.message.length() + 100);
    
    int pos = 0;
    while (pos < layout.length()) {
        char c = layout.at(pos++);
        if (c != '%') {
            message.append(c);
            continue;
        }
        
        QByteArray dig;
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
            
            if (c == 'd') {  // %d : timestamp
                if (!dateTimeFormat.isEmpty()) {
                    message.append(log.timestamp.toString(dateTimeFormat).toLatin1());
                } else {
                    message.append(log.timestamp.toString(Qt::ISODate).toLatin1());
                }
            } else if (c == 'p' || c == 'P') {  // %p or %P : priority
                QByteArray pri = priorityToString((TLogger::Priority)log.priority);
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
                
            } else if (c == 't' || c == 'T') {  // %t or %T : thread ID (dec or hex)
                QChar fillChar = (dig.length() > 0 && dig[0] == '0') ? QLatin1Char('0') : QLatin1Char(' ');
                message.append(QString("%1").arg((qulonglong)log.threadId, dig.toInt(), ((c == 't') ? 10 : 16), fillChar).toLatin1());
                
            } else if (c == 'i' || c == 'I') {  // %i or %I : PID (dec or hex)
                QChar fillChar = (dig.length() > 0 && dig[0] == '0') ? QLatin1Char('0') : QLatin1Char(' ');
                message.append(QString("%1").arg(log.pid, dig.toInt(), ((c == 'i') ? 10 : 16), fillChar).toLatin1());
                
            } else if (c == 'n') {  // %n : newline
                message.append('\n');
            } else if (c == 'm') {  // %m : message
                message.append(log.message);
            } else if (c == '%') {
                message.append('%').append(dig);
                dig.clear();
                continue;
            } else {
                message.append('%').append(dig).append(c);
            }
            break;
        }
    }

    return (codec) ? codec->fromUnicode(QString::fromLocal8Bit(message.data(), message.length())) : message;
}


QByteArray TLogger::priorityToString(Priority priority)
{
    return priorityHash()->value(priority);
}


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
    threshold_ = priorityHash()->key(pri, TLogger::Trace);

    QFileInfo fi(settingsValue("Target", "log/app.log").toString());
    target_ = (fi.isAbsolute()) ? fi.absoluteFilePath() : Tf::app()->webRootPath() + fi.filePath();

    QDir dir = QFileInfo(target_).dir();
    if (!dir.exists()) {
        // Created a directory
        dir.mkpath(".");
    }
}
