#pragma once
//#include <TActionContext>
//#include "tthreadpool.h"
#include <QFile>
#include <coroutine>

class TUringCoroutine;
class THttpRequest;
struct TUringTask;


//class TUringCoroutine : public TActionContext {
class TUringCoroutine {
public:
    // TUringCoroutine(int socketDescriptor) :
    //     TActionContext(), _sd(socketDescriptor) {}
    TUringCoroutine(int socketDescriptor) :
        _sd(socketDescriptor) {}
    virtual ~TUringCoroutine();

    TUringTask start();
    //TAsyncTask<int> executeRequest(THttpRequest &request);

// protected:
//     virtual int64_t writeResponse(THttpResponseHeader &, QIODevice *) override;

private:
    int _sd {0};
    QByteArray _response;
    QString _fileName;
};
