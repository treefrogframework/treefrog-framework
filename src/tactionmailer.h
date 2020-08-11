#pragma once
#include <TAbstractController>


class T_CORE_EXPORT TActionMailer : public TAbstractController {
public:
    TActionMailer();
    virtual ~TActionMailer() { }
    virtual QString name() const;
    virtual QString activeAction() const;

protected:
    bool deliver(const QString &templateName = "mail");

private:
    T_DISABLE_COPY(TActionMailer)
    T_DISABLE_MOVE(TActionMailer)
};

