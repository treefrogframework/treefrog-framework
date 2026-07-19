#pragma once
#include <TActionContext>

class THttpRequest;


class T_CORE_EXPORT TActionContextRoutine : public TActionContext {
public:
    TActionContextRoutine() = default;
    ~TActionContextRoutine() = default;
    void start(THttpRequest &request);

    class Result {
    public:
        QByteArray response;
        QString fileName;
    } result;

protected:
    virtual int64_t writeResponse(THttpResponseHeader &, QIODevice *) override;

    T_DISABLE_COPY(TActionContextRoutine)
    T_DISABLE_MOVE(TActionContextRoutine)
};
