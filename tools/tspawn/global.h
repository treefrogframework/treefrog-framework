#pragma once
#include <QDir>
#include <QString>

extern QString fieldNameToVariableName(const QString &name);
extern QString fieldNameToEnumName(const QString &name);
extern QString enumNameToVariableName(const QString &name);

extern QString fieldNameToCaption(const QString &name);
extern QString enumNameToCaption(const QString &name);

extern bool mkpath(const QDir &dir, const QString &dirPath = ".");

