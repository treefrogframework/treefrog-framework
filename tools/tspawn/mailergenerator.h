#pragma once
#include <QDir>
#include <QString>


class MailerGenerator {
public:
    MailerGenerator(const QString &name, const QStringList &actions);
    bool generate(const QString &dst) const;

private:
    QString mailerName;
    QStringList actionList;
};

