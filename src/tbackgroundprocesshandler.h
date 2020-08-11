#pragma once
#include <QProcess>
#include <TDatabaseContext>
#include <TGlobal>

class TBackgroundProcess;


class T_CORE_EXPORT TBackgroundProcessHandler : public QObject, public TDatabaseContext {
    Q_OBJECT
public:
    TBackgroundProcessHandler(QObject *parent = nullptr);
    virtual ~TBackgroundProcessHandler() { }

    bool autoDelete() const;
    void setAutoDelete(bool autoDelete);
    TBackgroundProcess *backgroundProcess() const { return _process; }

protected slots:
    virtual void handleFinished(int exitCode, QProcess::ExitStatus exitStatus) = 0;
    virtual void handleReadyReadStandardError() { }
    virtual void handleReadyReadStandardOutput() { }
    virtual void handleStarted() { }
    virtual void handleStateChanged(QProcess::ProcessState) { }

#if QT_VERSION >= 0x050600
    virtual void handleErrorOccurred(QProcess::ProcessError)
    {
    }
#endif

    void deleteAutoDeleteHandler();

private:
    TBackgroundProcess *_process {nullptr};
    bool _autoDelete {true};

    friend class TBackgroundProcess;
    T_DISABLE_COPY(TBackgroundProcessHandler)
    T_DISABLE_MOVE(TBackgroundProcessHandler)
};

