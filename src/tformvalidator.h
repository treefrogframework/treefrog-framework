#ifndef TFORMVALIDATOR_H
#define TFORMVALIDATOR_H

#include <QStringList>
#include <QRegExp>
#include <QPair>
#include <QVariant>
#include <TGlobal>


class T_CORE_EXPORT TFormValidator
{
public:
    TFormValidator() { }
    virtual ~TFormValidator() { }

    void setRule(const QString &key, Tf::ValidationRule rule, bool enable = true, const QString &errorMessage = QString());
    void setRule(const QString &key, Tf::ValidationRule rule, const QString &errorMessage);
    void setRule(const QString &key, Tf::ValidationRule rule, const char *errorMessage);
    void setRule(const QString &key, Tf::ValidationRule rule, float val, const QString &errorMessage = QString());
    void setRule(const QString &key, Tf::ValidationRule rule, double val, const QString &errorMessage = QString());
    void setRule(const QString &key, Tf::ValidationRule rule, int val, const QString &errorMessage = QString());
    void setRule(const QString &key, Tf::ValidationRule rule, qint64 val, const QString &errorMessage = QString());
    void setPatternRule(const QString &key, const QRegExp &rx, const QString &errorMessage = QString());
    QString message(const QString &key, Tf::ValidationRule rule) const;

    void setDateFormat(const QString &format);
    QString dateFormat() const;
    void setTimeFormat(const QString &format);
    QString timeFormat() const;
    void setDateTimeFormat(const QString &format);
    QString dateTimeFormat() const;

    virtual bool validate(const QVariantMap &map);
    QStringList validationErrorKeys() const;
    Tf::ValidationRule errorRule(const QString &key) const;
    QString errorMessage(const QString &key) const;
    QStringList errorMessages() const;

protected:
    class RuleEntry
    {
    public:
        QString key;
        int rule;
        QVariant value;
        QString message;

        RuleEntry(const QString &key, int rule, bool enable, const QString &errorMessage);
        RuleEntry(const QString &key, int rule, qint64 val, const QString &errorMessage);
        RuleEntry(const QString &key, int rule, double val, const QString &errorMessage);
        RuleEntry(const QString &key, int rule, const QRegExp &rx, const QString &errorMessage);
    };

    bool containsRule(const QString &key, Tf::ValidationRule rule) const;
    void removeRule(const QString &key, Tf::ValidationRule rule);
    void setValidationError(const QString &errorMessage);

    QList<RuleEntry> rules;
    QList<QPair<QString, int>> errors;

private:
    QString dateFmt;
    QString timeFmt;
    QString dateTimeFmt;
};


inline void TFormValidator::setDateFormat(const QString &format)
{ dateFmt = format; }

inline void TFormValidator::setTimeFormat(const QString &format)
{ timeFmt = format; }

inline void TFormValidator::setDateTimeFormat(const QString &format)
{ dateTimeFmt = format; }

#endif // TFORMVALIDATOR_H
