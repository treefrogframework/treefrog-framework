#ifndef ERBCONVERTER_H
#define ERBCONVERTER_H

#include <QString>
#include <QFile>
#include <QDir>

class ErbParser;


class ErbConverter
{
public:
    ErbConverter(const QDir &output, const QDir &helpers);
    bool convert(const QString &erbPath, int trimMode) const;
    bool convert(const QString &className, const QString &erb) const;
    QDir outputDir() const { return outputDirectory; }
    //static QString convertToSourceCode(const QString &className, const QString &erb);
    static QString fileSuffix() { return "erb"; }
    static QString escapeNewline(const QString &string);

protected:
    QString generateIncludeCode(const ErbParser &parser) const;

private:
    QDir outputDirectory;
    QDir helpersDirectory;
};

#endif // ERBCONVERTER_H
