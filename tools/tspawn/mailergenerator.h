#ifndef MAILERGENERATOR_H
#define MAILERGENERATOR_H

#include <QString>
#include <QDir>


class MailerGenerator
{
public:
    MailerGenerator(const QString &name, const QStringList &actions, const QString &dst);
    bool generate() const;

private:
    QString mailerName;
    QStringList actionList;
    QDir dstDir;
};

#endif // MAILERGENERATOR_H
