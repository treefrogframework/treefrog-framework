/* Copyright (c) 2011-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <TAbstractUser>
#include <TAccessValidator>
#include <TActionContext>
#include <TActionController>
#include <TSystemGlobal>


/*!
  \class TAccessValidator
  The TAccessValidator class provides validation of user access.
  \sa TAbstractUser class
*/

/*!
  Constructor.
*/
TAccessValidator::TAccessValidator()
{
}

/*!
  Sets to allow a group with \a groupKey to access to the action \a action.
*/
void TAccessValidator::setAllowGroup(const QString &groupKey, const QString &action)
{
    accessRules << AccessRule(AccessRule::Group, groupKey, action, true);
}

/*!
  Sets to allow a group with \a groupKey to access to the actions \a actions.
*/
void TAccessValidator::setAllowGroup(const QString &groupKey, const QStringList &actions)
{
    addRules(AccessRule::Group, groupKey, actions, true);
}

/*!
  Sets to deny a group with \a groupKey to access to the action \a action.
*/
void TAccessValidator::setDenyGroup(const QString &groupKey, const QString &action)
{
    accessRules << AccessRule(AccessRule::Group, groupKey, action, false);
}

/*!
  Sets to deny a group with \a groupKey to access to the actions \a actions.
*/
void TAccessValidator::setDenyGroup(const QString &groupKey, const QStringList &actions)
{
    addRules(AccessRule::Group, groupKey, actions, false);
}

/*!
  Sets to allow a user with the identity \a identityKey to access to
  the action \a action.
*/
void TAccessValidator::setAllowUser(const QString &identityKey, const QString &action)
{
    accessRules << AccessRule(AccessRule::User, identityKey, action, true);
}

/*!
  Sets to allow a user with the identity \a identityKey to access to
  the actions \a actions.
*/
void TAccessValidator::setAllowUser(const QString &identityKey, const QStringList &actions)
{
    addRules(AccessRule::User, identityKey, actions, true);
}

/*!
  Sets to deny a user with the identity \a identityKey to access to
  the action \a action.
*/
void TAccessValidator::setDenyUser(const QString &identityKey, const QString &action)
{
    accessRules << AccessRule(AccessRule::User, identityKey, action, false);
}

/*!
  Sets to deny a user with the identity \a identityKey to access to
  the actions \a actions.
*/
void TAccessValidator::setDenyUser(const QString &identityKey, const QStringList &actions)
{
    addRules(AccessRule::User, identityKey, actions, false);
}

/*!
  Sets to allow an unauthenticated user with the identity \a identityKey
  to access to the action \a action.
*/
void TAccessValidator::setAllowUnauthenticatedUser(const QString &action)
{
    accessRules << AccessRule(AccessRule::UnauthenticatedUser, QString(), action, true);
}

/*!
  Sets to allow an unauthenticated with the identity \a identityKey to
  access to the actions \a actions.
*/
void TAccessValidator::setAllowUnauthenticatedUser(const QStringList &actions)
{
    addRules(AccessRule::UnauthenticatedUser, QString(), actions, true);
}

/*!
  Sets to deny an unauthenticated with the identity \a identityKey to
  access to the action \a action.
*/
void TAccessValidator::setDenyUnauthenticatedUser(const QString &action)
{
    accessRules << AccessRule(AccessRule::UnauthenticatedUser, QString(), action, false);
}

/*!
  Sets to deny an unauthenticated with the identity \a identityKey to
  access to the actions \a actions.
*/
void TAccessValidator::setDenyUnauthenticatedUser(const QStringList &actions)
{
    addRules(AccessRule::UnauthenticatedUser, QString(), actions, false);
}

/*!
  Added a access rule to the list.
*/
void TAccessValidator::addRules(int type, const QString &key, const QStringList &actions, bool allow)
{
    for (auto &act : actions) {
        accessRules << AccessRule(type, key, act, allow);
    }
}

/*!
  Returns true if the user \a user is allowed to access to the requested
  action; otherwise returns false.
*/
bool TAccessValidator::validate(const TAbstractUser *user) const
{
    bool ret = allowDefault;
    const TActionController *controller = Tf::constCurrentContext()->currentController();
    Q_ASSERT(controller);

    if (accessRules.isEmpty()) {
        tWarn("No rule for access validation: %s", qUtf8Printable(controller->className()));
        return ret;
    }

    if (!user || user->identityKey().isEmpty()) {
        // Searches a access rule for an unauthenticated user
        for (const auto &rule : accessRules) {
            if (rule.type == AccessRule::UnauthenticatedUser
                && rule.action == controller->activeAction()) {
                ret = rule.allow;
                break;
            }
        }
        if (ret) {
            tSystemDebug("Access '%s' action by an unauthenticated user : Allow", qUtf8Printable(controller->activeAction()));
        } else {
            tSystemWarn("Access '%s' action by an unauthenticated user : Deny", qUtf8Printable(controller->activeAction()));
        }

    } else {
        for (const auto &rule : accessRules) {
            if (rule.action == controller->activeAction()
                && ((rule.type == AccessRule::User && rule.key == user->identityKey())
                    || (!user->groupKey().isEmpty() && rule.key == user->groupKey()))) {
                ret = rule.allow;
                break;
            }
        }
        if (ret) {
            tSystemDebug("Access '%s' action by '%s' user : Allow", qUtf8Printable(controller->activeAction()), qUtf8Printable(user->identityKey()));
        } else {
            tSystemWarn("Access '%s' action by '%s' user : Deny", qUtf8Printable(controller->activeAction()), qUtf8Printable(user->identityKey()));
        }
    }

    return ret;
}

/*!
  Removes all access rules from the list.
*/
void TAccessValidator::clear()
{
    accessRules.clear();
    allowDefault = true;
}


/*!
  \fn void TAccessValidator::setAllowDefault(bool allow)
  Sets the default rule to allow all users to access to all actions
  if \a allow is true; otherwise sets to deny any user to access
  to any action. The default rule is true.
*/

/*!
  \fn void TAccessValidator::setDenyDefault(bool deny)
  Sets the default rule to deny any user to access to any action
  if \a deny is true; otherwise sets to allow all users to access
  to all actions.
*/

/*!
  \class TAccessValidator::AccessRule
  The AccessRule class is for internal use only.
*/
