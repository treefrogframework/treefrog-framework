#ifndef TBACKGROUNDPROCESSHANDLER_H
#define TBACKGROUNDPROCESSHANDLER_H

#include <QProcess>
#include <TGlobal>
#include <TDatabaseContext>


class T_CORE_EXPORT TBackgroundProcessHandler : public QObject, public TDatabaseContext
{
    Q_OBJECT
public:
    TBackgroundProcessHandler(QObject *parent = nullptr);
    virtual ~TBackgroundProcessHandler() {}

    bool autoDelete() const;
    void setAutoDelete(bool autoDelete);

protected slots:
    virtual void handleFinished(int exitCode, QProcess::ExitStatus exitStatus) = 0;
    virtual void handleReadyReadStandardError() {}
    virtual void handleReadyReadStandardOutput() {}
    virtual void handleStarted() {}
    virtual void handleStateChanged(QProcess::ProcessState) {}

#if QT_VERSION >= 0x050600
    virtual void handleErrorOccurred(QProcess::ProcessError) {}
#endif

    void deleteAutoDeleteHandler();

private:
    bool _autoDelete {true};
};

#endif // BACKGROUNDPROCESSHANDLER_H
