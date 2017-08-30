#ifndef ERBCONVERTER_H
#define ERBCONVERTER_H

#include <QString>
#include <QFile>
#include <QDir>

class ErbParser;


class ErbConverter
{
public:
    ErbConverter(const QDir &output, const QDir &partial);
    bool convert(const QString &erbPath, int trimMode) const;
    bool convert(const QString &className, const QString &erb, int trimMode) const;
    QDir outputDir() const { return outputDirectory; }
    static QString fileSuffix() { return "erb"; }
    static QString escapeNewline(const QString &string);

protected:
    QString generateIncludeCode(const ErbParser &parser) const;
    QStringList replacePartialTag(QString &erb, int depth) const;

private:
    QDir outputDirectory;
    QDir partialDirectory;
};

#endif // ERBCONVERTER_H
