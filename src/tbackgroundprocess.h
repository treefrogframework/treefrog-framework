#ifndef TBACKGROUNDPROCESS_H
#define TBACKGROUNDPROCESS_H

#include <QProcess>
#include <TGlobal>
#include <TBackgroundProcessHandler>


class T_CORE_EXPORT TBackgroundProcess : public QProcess
{
    Q_OBJECT
public:
    TBackgroundProcess(QObject *parent = nullptr);
    virtual ~TBackgroundProcess() {}

    void start(const QString &program, const QStringList &arguments, OpenMode mode = ReadWrite, TBackgroundProcessHandler *handler = nullptr);
    void start(const QString &command, OpenMode mode = ReadWrite, TBackgroundProcessHandler *handler = nullptr);
    void start(OpenMode mode = ReadWrite, TBackgroundProcessHandler *handler = nullptr);

protected slots:
    void callStart(const QString &program, const QStringList &arguments, int mode);

protected:
    void connectToSlots(TBackgroundProcessHandler *handler);
};

#endif // BACKGROUNDPROCESS_H
