#ifndef TSCHEDULER_H
#define TSCHEDULER_H

#include <TGlobal>
#include <TDatabaseContextThread>
#include <QTimer>


class T_CORE_EXPORT TScheduler : public TDatabaseContextThread
{
    Q_OBJECT
public:
    TScheduler(QObject *parent = nullptr);
    virtual ~TScheduler();

    void start(int msec);
    void restart();
    void stop();
    int	interval() const;
    bool isSingleShot() const;
    void setSingleShot(bool singleShot);

protected:
    virtual void job() = 0;
    void rollbackTransaction();
    void publish(const QString &topic, const QString &text);
    void publish(const QString &topic, const QByteArray &binary);

private slots:
    void start(Priority priority = InheritPriority);

signals:
    void startTimer(int msec);
    void startTimer();
    void stopTimer();

private:
    void run() override;

    QTimer *timer {nullptr};
    bool rollback {false};

    T_DISABLE_COPY(TScheduler)
    T_DISABLE_MOVE(TScheduler)
};

#endif // TSCHEDULER_H
