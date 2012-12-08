#ifndef TMAILER_H
#define TMAILER_H

#include <QString>
#include <TGlobal>
#include <TMailMessage>


class T_CORE_EXPORT TMailer
{
public:
    virtual ~TMailer() { }
    virtual QString key() const = 0;
    virtual bool send(const TMailMessage &message) = 0;
};

#endif // TMAILER_H
