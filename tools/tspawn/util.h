#pragma once
#include <QByteArray>
#include <QDir>
#include <QString>

extern QString dataDirPath;

extern void copy(const QString &src, const QString &dst);
extern void copy(const QString &src, const QDir &dst);
extern bool rmpath(const QString &path);
extern bool remove(const QString &file);
extern bool remove(QFile &file);
extern bool replaceString(const QString &fileName, const QByteArray &before, const QByteArray &after);

