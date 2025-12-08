#pragma once
#include <TActionContext>


class T_CORE_EXPORT TActionContextRoutine : public TActionContext {
public:
    TActionContextRoutine() = default;
    ~TActionContextRoutine() = default;
    void start(QByteArray &readBuffer);

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
