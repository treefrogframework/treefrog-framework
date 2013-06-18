#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <QDir>

QString fieldNameToVariableName(const QString &name);
QString fieldNameToEnumName(const QString &name);
QString enumNameToVariableName(const QString &name);

QString fieldNameToCaption(const QString &name);
QString enumNameToCaption(const QString &name);

bool mkpath(const QDir &dir, const QString &dirPath = ".");

#endif // GLOBAL_H
