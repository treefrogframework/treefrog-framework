#ifndef OTAMACONVERTER_H
#define OTAMACONVERTER_H

#include <QString>
#include <QDir>
#include "erbconverter.h"


class OtamaConverter
{
public:
    OtamaConverter(const QDir &output, const QDir &helpers);
    ~OtamaConverter();

    bool convert(const QString &filePath) const;
    static QString convertToErb(const QString &html, const QString &otm);
    static QString fileSuffix() { return "html"; }
    static QString logicFileSuffix() { return "otm"; }

private:
    Q_DISABLE_COPY(OtamaConverter)

    ErbConverter erbConverter;
};

#endif // OTAMACONVERTER_H
