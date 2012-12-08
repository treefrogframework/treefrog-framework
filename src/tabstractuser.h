#ifndef TABSTRACTUSER_H
#define TABSTRACTUSER_H

#include <QString>
#include <TGlobal>


class T_CORE_EXPORT TAbstractUser
{
public:
    virtual ~TAbstractUser() { }
    virtual QString identityKey() const = 0;
    virtual QString groupKey() const { return QString(); }
};

#endif // TABSTRACTUSER_H
