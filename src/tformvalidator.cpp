/* Copyright (c) 2011-2015, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QUrl>
#include <QLocale>
#include <QDateTime>
#include <TFormValidator>
#include <TWebApplication>
#include "tsystemglobal.h"

#define ATOM       "[a-zA-Z0-9_!#\\$\\%&'*+/=?\\^`{}~|\\-]+"
#define DOT_ATOM   ATOM "(?:\\." ATOM ")*"
#define QUOTED     "\\\"(?:[\\x20\\x23-\\x5b\\x5d-\\x7e]|\\\\[\\x20-\\x7e])*\\\""
#define LOCAL      "(?:" DOT_ATOM "|" QUOTED ")"
#define ADDR_SPEC  LOCAL "@" DOT_ATOM

/*!
  \class TFormValidator::RuleEntry
  \brief The RuleEntry class is for internal use only.
*/

TFormValidator::RuleEntry::RuleEntry(const QString &k, int r, bool enable, const QString &msg)
    : key(k), rule(r), value(enable), message(msg)
{ }


TFormValidator::RuleEntry::RuleEntry(const QString &k, int r, qint64 v, const QString &msg)
    : key(k), rule(r), value(v), message(msg)
{ }


TFormValidator::RuleEntry::RuleEntry(const QString &k, int r, double v, const QString &msg)
    : key(k), rule(r), value(v), message(msg)
{ }


TFormValidator::RuleEntry::RuleEntry(const QString &k, int r, const QRegExp &rx, const QString &msg)
    : key(k), rule(r), value(rx), message(msg)
{ }


/*!
  \class TFormValidator
  \brief The TFormValidator class provides form validation for
  a map-table-based dictionary.
*/

/*!
  Returns the date format specified by the validation settings file.
*/
QString TFormValidator::dateFormat() const
{
    return (dateFmt.isEmpty()) ? Tf::app()->validationSettings().value("DateFormat").toString() : dateFmt;
}

/*!
  Returns the time format specified by the validation settings file.
*/
QString TFormValidator::timeFormat() const
{
    return (timeFmt.isEmpty()) ? Tf::app()->validationSettings().value("TimeFormat").toString() : timeFmt;
}

/*!
  Returns the format of the date and time specified by the
  validation settings file.
*/
QString TFormValidator::dateTimeFormat() const
{
    return (dateTimeFmt.isEmpty()) ? Tf::app()->validationSettings().value("DateTimeFormat").toString() : dateTimeFmt;
}

/*!
  Sets the validation rule for the key \a key and sets the error
  message of it to \a errorMessage.

  If \a enable is true (by default), the rule is enabled.
 */
void TFormValidator::setRule(const QString &key, Tf::ValidationRule rule, bool enable, const QString &errorMessage)
{
    // arg check
    switch ((int)rule) {
    case Tf::MaxLength:
    case Tf::MinLength:
    case Tf::IntMax:
    case Tf::IntMin:
    case Tf::DoubleMax:
    case Tf::DoubleMin:
        tWarn("Validation: Bad rule spedified [key:%s  rule:%d]. Use another setRule method.", qPrintable(key), rule);
        return;

    case Tf::Pattern:
        tWarn("Validation: Bad rule spedified [key:%s  rule:%d]. Use setPatternRule method.", qPrintable(key), rule);
        return;
    }

    removeRule(key, rule);
    rules.prepend(RuleEntry(key, (int)rule, enable, (errorMessage.isEmpty() ? Tf::app()->validationErrorMessage(rule) : errorMessage)));
}

/*!
  Sets the validation rule for the key \a key and sets the error
  message of it to \a errorMessage.
 */
void TFormValidator::setRule(const QString &key, Tf::ValidationRule rule, int val, const QString &errorMessage)
{
    setRule(key, rule, (qint64)val, errorMessage);
}

/*!
  Sets the validation rule for the key \a key and sets the error
  message of it to \a errorMessage.
 */
void TFormValidator::setRule(const QString &key, Tf::ValidationRule rule, qint64 val, const QString &errorMessage)
{
    // arg check
    switch ((int)rule) {
    case Tf::Required:
    case Tf::EmailAddress:
    case Tf::Url:
    case Tf::Date:
    case Tf::Time:
    case Tf::DateTime:
        tWarn("Validation: Bad rule spedified [key:%s  rule:%d]. Use another setRule method.", qPrintable(key), rule);
        return;

    case Tf::Pattern:
        tWarn("Validation: Bad rule spedified [key:%s  rule:%d]. Use setPatternRule method.", qPrintable(key), rule);
        return;
    }

    removeRule(key, rule);
    rules.prepend(RuleEntry(key, (int)rule, val, (errorMessage.isEmpty() ? Tf::app()->validationErrorMessage(rule) : errorMessage)));
}

/*!
  Sets the validation rule for the key \a key and set the error
  message of it to \a errorMessage.
 */
void TFormValidator::setRule(const QString &key, Tf::ValidationRule rule, float val, const QString &errorMessage)
{
    setRule(key, rule, (double)val, errorMessage);
}

/*!
  Sets the validation rule for the key \a key and sets the error
  message of it to \a errorMessage.
 */
void TFormValidator::setRule(const QString &key, Tf::ValidationRule rule, double val, const QString &errorMessage)
{
    // arg check
    switch ((int)rule) {
    case Tf::Required:
    case Tf::MaxLength:
    case Tf::MinLength:
    case Tf::IntMax:
    case Tf::IntMin:
    case Tf::EmailAddress:
    case Tf::Url:
    case Tf::Date:
    case Tf::Time:
    case Tf::DateTime:
        tWarn("Validation: Bad rule spedified [key:%s  rule:%d]. Use another setRule method.", qPrintable(key), rule);
        return;

    case Tf::Pattern:
        tWarn("Validation: Bad rule spedified [key:%s  rule:%d]. Use setPatternRule method.", qPrintable(key), rule);
        return;
    }

    removeRule(key, rule);
    rules.prepend(RuleEntry(key, (int)rule, val, (errorMessage.isEmpty() ? Tf::app()->validationErrorMessage(rule) : errorMessage)));
}

/*!
  Sets the validation rule for the key \a key and sets the error
  message of it to \a errorMessage.
 */
void TFormValidator::setRule(const QString &key, Tf::ValidationRule rule, const QString &errorMessage)
{
    setRule(key, rule, true, errorMessage);
}

/*!
  Sets the validation rule for the key \a key and sets the error
  message of it to \a errorMessage.
 */
void TFormValidator::setRule(const QString &key, Tf::ValidationRule rule, const char *errorMessage)
{
    setRule(key, rule, true, QObject::tr(errorMessage));
}

/*!
  Sets the user-defined validaton rule for the key \a key and sets
  the error message of it to \a errorMessage.
 */
void TFormValidator::setPatternRule(const QString &key, const QRegExp &rx, const QString &errorMessage)
{
    removeRule(key, Tf::Pattern);
    rules.prepend(RuleEntry(key, Tf::Pattern, rx, (errorMessage.isEmpty() ? Tf::app()->validationErrorMessage(Tf::Pattern) : errorMessage)));
}

/*!
  Validates the specified parameter \a map by the set rules.

  As default, TF::Required is set for all parameters. If not required,
  set the rule to \a false like this:
    setRule("xxx", Tf::Required, false);
 */
bool TFormValidator::validate(const QVariantMap &map)
{
    errors.clear();

    // Add default rules, Tf::Required.
    QString msg = Tf::app()->validationErrorMessage(Tf::Required);
    for (QStringListIterator i(map.keys()); i.hasNext(); ) {
        const QString &k = i.next();
        if (!containsRule(k, Tf::Required)) {
            rules.append(RuleEntry(k, (int)Tf::Required, true, msg));
        }
    }

    for (QListIterator<RuleEntry> i(rules); i.hasNext(); ) {
        const RuleEntry &r = i.next();
        QString str = map.value(r.key).toString();  // value string

        if (str.isEmpty()) {
            bool req = r.value.toBool();
            if (r.rule == Tf::Required && req) {
                tSystemDebug("validation error: required parameter is empty, key:%s", qPrintable(r.key));
                errors << qMakePair(r.key, r.rule);
            }
        } else {
            bool ok1, ok2;
            tSystemDebug("validating key:%s value: %s", qPrintable(r.key), qPrintable(str));
            switch (r.rule) {
            case Tf::Required:
                break;

            case Tf::MaxLength: {
                int max = r.value.toInt(&ok2);
                if (!ok2 || str.length() > max) {
                    errors << qMakePair(r.key, r.rule);
                }
                break; }

            case Tf::MinLength: {
                int min = r.value.toInt(&ok2);
                if (!ok2 || str.length() < min) {
                    errors << qMakePair(r.key, r.rule);
                }
                break; }

            case Tf::IntMax: {
                qint64 n = str.toLongLong(&ok1);
                qint64 max = r.value.toLongLong(&ok2);
                if (!ok1 || !ok2 || n > max) {
                    errors << qMakePair(r.key, r.rule);
                }
                break; }

            case Tf::IntMin: {
                qint64 n = str.toLongLong(&ok1);
                qint64 min = r.value.toLongLong(&ok2);
                if (!ok1 || !ok2  || n < min) {
                    errors << qMakePair(r.key, r.rule);
                }
                break; }

            case Tf::DoubleMax: {
                double n = str.toDouble(&ok1);
                double max = r.value.toLongLong(&ok2);
                if (!ok1 || !ok2 || n > max) {
                    errors << qMakePair(r.key, r.rule);
                }
                break; }

            case Tf::DoubleMin: {
                double n = str.toDouble(&ok1);
                double min = r.value.toDouble(&ok2);
                if (!ok1 || !ok2 || n < min) {
                    errors << qMakePair(r.key, r.rule);
                }
                break; }

            case Tf::EmailAddress: { // refer to RFC5321
                if ( r.value.toBool() ) {
                    QRegExp reg("^" ADDR_SPEC "$");
                    if (!reg.exactMatch(str)) {
                        errors << qMakePair(r.key, r.rule);
                    }
                }
                break; }

            case Tf::Url: {
                if ( r.value.toBool() ) {
                    QUrl url(str, QUrl::StrictMode);
                    if (!url.isValid()) {
                        errors << qMakePair(r.key, r.rule);
                    }
                }
                break; }

            case Tf::Date: {
                if ( r.value.toBool() ) {
                    QDate date = QLocale().toDate(str, dateFormat());
                    if (!date.isValid()) {
                        errors << qMakePair(r.key, r.rule);
                        tSystemDebug("Validation error: Date format: %s", qPrintable(dateFormat()));
                    }
                }
                break; }

            case Tf::Time: {
                if ( r.value.toBool() ) {
                    QTime time = QLocale().toTime(str, timeFormat());
                    if (!time.isValid()) {
                        errors << qMakePair(r.key, r.rule);
                        tSystemDebug("Validation error: Time format: %s", qPrintable(timeFormat()));
                    }
                }
                break; }

            case Tf::DateTime: {
                if ( r.value.toBool() ) {
                    QDateTime dt = QLocale().toDateTime(str, dateTimeFormat());
                    if (!dt.isValid()) {
                        errors << qMakePair(r.key, r.rule);
                        tSystemDebug("Validation error: DateTime format: %s", qPrintable(dateTimeFormat()));
                    }
                }
                break; }

            case Tf::Pattern: {
                QRegExp rx = r.value.toRegExp();
                if (rx.isEmpty() || !rx.exactMatch(str)) {
                    errors << qMakePair(r.key, r.rule);
                }
                break; }

            default:
                tSystemError("Internal Error, invalid rule: %d  [%s:%d]", r.rule, __FILE__, __LINE__);
                break;
            }
        }
    }
    return errors.isEmpty();
}

/*!
  Returns a list of all keys of validation errors.
  This function must be call after calling \a validate() function.
 */
QStringList TFormValidator::validationErrorKeys() const
{
    QStringList ret;
    for (QListIterator<QPair<QString, int>> it(errors); it.hasNext(); ) {
        ret << it.next().first;
    }
    return ret;
}

/*!
  Returns a rule of validation errors with \a key.
  This function must be call after calling \a validate() function.
*/
Tf::ValidationRule TFormValidator::errorRule(const QString &key) const
{
    for (QListIterator<QPair<QString, int>> it(errors); it.hasNext(); ) {
        const QPair<QString, int> &p = it.next();
        if (p.first == key) {
            return (Tf::ValidationRule)p.second;
        }
    }
    return Tf::Required;
}

/*!
  Returns a message with \a key and \a rule in the validation rules.
*/
QString TFormValidator::message(const QString &key, Tf::ValidationRule rule) const
{
    for (QListIterator<RuleEntry> i(rules); i.hasNext(); ) {
        const RuleEntry &r = i.next();
        if (r.key == key && r.rule == rule) {
            return r.message;
        }
    }
    return QString();
}

/*!
  Return a message of validation error with \a key.
  This function must be call after calling \a validate() function.
 */
QString TFormValidator::errorMessage(const QString &key) const
{
    return message(key, errorRule(key));
}

/*!
  Return messages of validation error. This function must be call after
  calling \a validate() function.
 */
QStringList TFormValidator::errorMessages() const
{
    QStringList msgs;
    for (QListIterator<QPair<QString, int>> it(errors); it.hasNext(); ) {
        const QPair<QString, int> &p = it.next();
        QString m = message(p.first, (Tf::ValidationRule)p.second);
        if (!m.isEmpty())
            msgs.prepend(m);
    }
    return msgs;
}

/*!
  Returns true if the rules contains an item with the key;
  otherwise returns false.
 */
bool TFormValidator::containsRule(const QString &key, Tf::ValidationRule rule) const
{
    for (QListIterator<RuleEntry> i(rules); i.hasNext(); ) {
        const RuleEntry &r = i.next();
        if (r.key == key && r.rule == rule) {
            return true;
        }
    }
    return false;
}

/*!
  Removes the specified item with the given \a key and \a rule from
  the validation rules.
*/
void TFormValidator::removeRule(const QString &key, Tf::ValidationRule rule)
{
    for (QMutableListIterator<RuleEntry> i(rules); i.hasNext(); ) {
        const RuleEntry &r = i.next();
        if (r.key == key && r.rule == rule) {
            i.remove();
        }
    }
}

/*!
  Sets the message of custom validation error to \a errorMessage.
*/
void TFormValidator::setValidationError(const QString &errorMessage)
{
    errors << qMakePair(QString("_CustomValidationError"), (int)Tf::Custom);
    rules.append(RuleEntry("_CustomValidationError", Tf::Custom, "dummy", errorMessage));
}

/*!
  \fn TFormValidator::TFormValidator()
  Constructor.
*/

/*!
  \fn virtual TFormValidator::~TFormValidator()
  Destructor.
*/

/*!
  \fn void TFormValidator::setDateFormat(const QString &format)
  Sets the date format to \a format for validation.
*/

/*!
  \fn void TFormValidator::setTimeFormat(const QString &format)
  Sets the time format to \a format for validation.
*/

/*!
  \fn void TFormValidator::setDateTimeFormat(const QString &format)
  Sets the date and time format to \a format for validation.
*/
