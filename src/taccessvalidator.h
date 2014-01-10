#ifndef TACCESSVALIDATOR_H
#define TACCESSVALIDATOR_H

#include <QString>
#include <QStringList>
#include <QList>
#include <TGlobal>

class TAbstractUser;


class T_CORE_EXPORT TAccessValidator
{
public:
    TAccessValidator();
    virtual ~TAccessValidator() { }

    void setAllowDefault(bool allow = true) { allowDefault = allow; }
    void setDenyDefault(bool deny = true) { allowDefault = !deny; }
    void setAllowGroup(const QString &groupKey, const QString &action);
    void setAllowGroup(const QString &groupKey, const QStringList &actions);
    void setDenyGroup(const QString &groupKey, const QString &action);
    void setDenyGroup(const QString &groupKey, const QStringList &actions);
    void setAllowUser(const QString &identityKey, const QString &action);
    void setAllowUser(const QString &identityKey, const QStringList &actions);
    void setDenyUser(const QString &identityKey, const QString &action);
    void setDenyUser(const QString &identityKey, const QStringList &actions);
    void setAllowUnauthenticatedUser(const QString &action);
    void setAllowUnauthenticatedUser(const QStringList &actions);
    void setDenyUnauthenticatedUser(const QString &action);
    void setDenyUnauthenticatedUser(const QStringList &actions);
    void clear();
    virtual bool validate(const TAbstractUser *user) const;

protected:
    void addRules(int type, const QString &key, const QStringList &actions, bool allow);

    class AccessRule
    {
    public:
        enum Type {
            Group = 0,
            User,
            UnauthenticatedUser,
        };

        AccessRule(int t, const QString &k, const QString &act, bool alw)
            : type(t), key(k), action(act), allow(alw) { }

        int type;
        QString key;
        QString action;
        bool allow;
    };

    bool allowDefault;
    QList<AccessRule> accessRules;
};

#endif // TACCESSVALIDATOR_H
