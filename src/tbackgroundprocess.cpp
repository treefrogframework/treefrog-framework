#include "tbackgroundprocess.h"
#include <TWebApplication>
#include <TBackgroundProcessHandler>
#include <QObject>
#include <QMetaType>


TBackgroundProcess::TBackgroundProcess(QObject *parent)
    : QProcess(parent)
{
    moveToThread(Tf::app()->databaseContextMainThread());
}


void TBackgroundProcess::connectToSlots(TBackgroundProcessHandler *handler)
{
    if (handler) {
        QObject::connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),   handler, SLOT(handleFinished(int, QProcess::ExitStatus)), Qt::QueuedConnection);
        QObject::connect(this, SIGNAL(readyReadStandardError()),              handler, SLOT(handleReadyReadStandardError()), Qt::QueuedConnection);
        QObject::connect(this, SIGNAL(readyReadStandardOutput()),             handler, SLOT(handleReadyReadStandardOutput()), Qt::QueuedConnection);
        QObject::connect(this, SIGNAL(started()),                             handler, SLOT(handleStarted()), Qt::QueuedConnection);
        QObject::connect(this, SIGNAL(stateChanged(QProcess::ProcessState)),  handler, SLOT(handleStateChanged(QProcess::ProcessState)), Qt::QueuedConnection);
#if QT_VERSION >= 0x050600
        QObject::connect(this, SIGNAL(errorOccurred(QProcess::ProcessError)), handler, SLOT(handleErrorOccurred(QProcess::ProcessError)), Qt::QueuedConnection);
#endif
        QObject::connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),   handler, SLOT(deleteAutoDeleteHandler()), Qt::QueuedConnection);

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


void TBackgroundProcess::callStart(const QString &program, const QStringList &arguments, int mode)
{
    if (program.isEmpty()) {
        QProcess::start((OpenMode)mode);
    } else if (arguments.isEmpty()) {
        QProcess::start(program, (OpenMode)mode);
    } else {
        QProcess::start(program, arguments, (OpenMode)mode);
    }
}

// RegisterMetaType - QProcess::ProcessState
Q_DECLARE_METATYPE(QProcess::ProcessState)

class StaticQProcessProcessStateRegisterMetaType
{
public:
    StaticQProcessProcessStateRegisterMetaType() { qRegisterMetaType<QProcess::ProcessState>(); }
};
static StaticQProcessProcessStateRegisterMetaType _staticQProcessProcessStateRegisterMetaType;
