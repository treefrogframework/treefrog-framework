#ifndef MAILERGENERATOR_H
#define MAILERGENERATOR_H

#include <QString>
#include <QDir>


class MailerGenerator
{
public:
    MailerGenerator(const QString &name, const QStringList &actions);
    bool generate(const QString &dst) const;

private:
    QString mailerName;
    QStringList actionList;
};

#endif // MAILERGENERATOR_H
