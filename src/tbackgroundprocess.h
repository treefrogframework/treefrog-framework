#pragma once
#include <QProcess>
#include <TBackgroundProcessHandler>
#include <TGlobal>


class T_CORE_EXPORT TBackgroundProcess : public QProcess {
    Q_OBJECT
public:
    TBackgroundProcess(QObject *parent = nullptr);
    virtual ~TBackgroundProcess() { }

    void start(const QString &program, const QStringList &arguments, OpenMode mode = ReadWrite, TBackgroundProcessHandler *handler = nullptr);
    void start(const QString &command, OpenMode mode = ReadWrite, TBackgroundProcessHandler *handler = nullptr);
    void start(OpenMode mode = ReadWrite, TBackgroundProcessHandler *handler = nullptr);
    bool autoDelete() const;
    void setAutoDelete(bool autoDelete);

protected slots:
    void callStart(const QString &program, const QStringList &arguments, int mode);
    void handleFinished();

private:
    void connectToSlots(TBackgroundProcessHandler *handler);

    bool _autoDelete {true};

    T_DISABLE_COPY(TBackgroundProcess)
    T_DISABLE_MOVE(TBackgroundProcess)
};

