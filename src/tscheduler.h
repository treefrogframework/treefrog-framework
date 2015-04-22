#ifndef TSCHEDULER_H
#define TSCHEDULER_H

#include <QTimer>
#include <QThread>
#include <TGlobal>
#include <TDatabaseContext>


class T_CORE_EXPORT TScheduler : public QThread, public TDatabaseContext
{
    Q_OBJECT
public:
    TScheduler(QObject *parent = 0);
    virtual ~TScheduler();

    void start(int msec);
    void stop();
    int	interval() const;
    bool isSingleShot() const;
    void setSingleShot(bool singleShot);

protected:
    virtual void job() = 0;
    void rollbackTransaction();

private slots:
    void start(Priority priority = InheritPriority);

private:
    void run();

    QTimer *timer;
    volatile bool rollback;

    Q_DISABLE_COPY(TScheduler)
};

#endif // TSCHEDULER_H
