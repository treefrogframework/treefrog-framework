#include "tbackgroundprocess.h"
#include <QMetaType>
#include <QObject>
#include <TBackgroundProcessHandler>
#include <TWebApplication>

/*!
  \class TBackgroundProcess
  \brief The TBackgroundProcess class is used to start external programs in background.
*/


TBackgroundProcess::TBackgroundProcess(QObject *parent) :
    QProcess(parent)
{
    moveToThread(Tf::app()->databaseContextMainThread());
}


void TBackgroundProcess::connectToSlots(TBackgroundProcessHandler *handler)
{
    if (handler) {
        QObject::connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), handler, SLOT(handleFinished(int, QProcess::ExitStatus)));
        QObject::connect(this, SIGNAL(readyReadStandardError()), handler, SLOT(handleReadyReadStandardError()));
        QObject::connect(this, SIGNAL(readyReadStandardOutput()), handler, SLOT(handleReadyReadStandardOutput()));
        QObject::connect(this, SIGNAL(started()), handler, SLOT(handleStarted()));
        QObject::connect(this, SIGNAL(stateChanged(QProcess::ProcessState)), handler, SLOT(handleStateChanged(QProcess::ProcessState)));
#if QT_VERSION >= 0x050600
        QObject::connect(this, SIGNAL(errorOccurred(QProcess::ProcessError)), handler, SLOT(handleErrorOccurred(QProcess::ProcessError)));
#endif
        QObject::connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), handler, SLOT(deleteAutoDeleteHandler()));
        QObject::connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(handleFinished()), Qt::QueuedConnection);

        handler->_process = this;
    }
}


void TBackgroundProcess::start(const QString &program, const QStringList &arguments, OpenMode mode, TBackgroundProcessHandler *handler)
{
    connectToSlots(handler);
    QMetaObject::invokeMethod(this, "callStart", Qt::QueuedConnection,
        Q_ARG(QString, program),
        Q_ARG(QStringList, arguments),
        Q_ARG(int, mode));
}


void TBackgroundProcess::start(const QString &command, OpenMode mode, TBackgroundProcessHandler *handler)
{
    connectToSlots(handler);
    QMetaObject::invokeMethod(this, "callStart", Qt::QueuedConnection,
        Q_ARG(QString, command),
        Q_ARG(QStringList, QStringList()),
        Q_ARG(int, mode));
}


void TBackgroundProcess::start(OpenMode mode, TBackgroundProcessHandler *handler)
{
    connectToSlots(handler);
    QMetaObject::invokeMethod(this, "callStart", Qt::QueuedConnection,
        Q_ARG(QString, QString()),
        Q_ARG(QStringList, QStringList()),
        Q_ARG(int, mode));
}


bool TBackgroundProcess::autoDelete() const
{
    return _autoDelete;
}


void TBackgroundProcess::setAutoDelete(bool autoDelete)
{
    _autoDelete = autoDelete;
}


void TBackgroundProcess::handleFinished()
{
    if (_autoDelete) {
        deleteLater();
    }
}


void TBackgroundProcess::callStart(const QString &program, const QStringList &arguments, int mode)
{
    if (program.isEmpty()) {
        QProcess::start((OpenMode)mode);
    } else {
        QProcess::start(program, arguments, (OpenMode)mode);
    }
}

// RegisterMetaType - QProcess::ProcessState
Q_DECLARE_METATYPE(QProcess::ProcessState)

class StaticQProcessProcessStateRegisterMetaType {
public:
    StaticQProcessProcessStateRegisterMetaType() { qRegisterMetaType<QProcess::ProcessState>(); }
};
static StaticQProcessProcessStateRegisterMetaType _staticQProcessProcessStateRegisterMetaType;
