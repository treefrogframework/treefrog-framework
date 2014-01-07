#ifndef TAPPLICATIONSCHEDULER_H
#define TAPPLICATIONSCHEDULER_H

#include <TScheduler>


class T_CORE_EXPORT TApplicationScheduler : public TScheduler
{
    Q_OBJECT
public:
    TApplicationScheduler(QObject *parent);
    virtual ~TApplicationScheduler();

    void start(int msec);
    void stop();
    int	interval() const;
    bool isSingleShot() const;
    void setSingleShot(bool singleShot);

protected:
    virtual void job() = 0;
    void rollbackTransaction();

private:
    Q_DISABLE_COPY(TApplicationScheduler)
};

#endif // TAPPLICATIONSCHEDULER_H
