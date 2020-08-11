#pragma once
#include "erbconverter.h"
#include <QDir>
#include <QString>
#include <TGlobal>


class OtamaConverter {
public:
    OtamaConverter(const QDir &output, const QDir &helpers, const QDir &partial);
    ~OtamaConverter();

    bool convert(const QString &filePath, int trimMode) const;
    static QString convertToErb(const QString &html, const QString &otm, int trimMode);
    static QString fileSuffix() { return "html"; }
    static QString logicFileSuffix() { return "otm"; }

private:
    ErbConverter erbConverter;

    T_DISABLE_COPY(OtamaConverter)
    T_DISABLE_MOVE(OtamaConverter)
};

