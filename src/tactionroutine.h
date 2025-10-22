#pragma once
#include <QObject>
#include <QBasicTimer>
#include <QMap>
#include <TGlobal>
#include <TActionContext>
#include <THttpRequest>
#include <THttpResponse>


class T_CORE_EXPORT TActionRoutine : public TActionContext
{
public:
    TActionRoutine();
    virtual ~TActionRoutine();

    THttpResponse start(THttpRequest &request);
    // static TActionRoutine *currentProcess();
    // static bool isChildProcess();

protected:
    virtual int64_t writeResponse(THttpResponseHeader &, QIODevice *) override;


//     virtual bool openDatabase();
//     virtual void closeDatabase();
//     virtual void emitError(int socketError);

// protected slots:
//     void terminate(int status);
//     void kill(int sig);

// signals:
//     void started();  // for the parent
//     void forked();   // for the child
//     void finished(); // for the parent and the child
//     void error(int socketError);

// private:
//     int childPid;
//     static TActionRoutine *currentActionRoutine;

//     friend class TActionRoutineManager;
    Q_DISABLE_COPY(TActionRoutine)
};
