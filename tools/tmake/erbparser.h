#pragma once
#include <QPair>
#include <QString>


class ErbParser {
public:
    enum TrimMode {
        TrimOff = 0,
        NormalTrim,  // Removes whitespaces if the start is "<%" and the end is "%>"
        StrongTrim,  // Removes whitespaces from the start and the end
    };

    ErbParser(TrimMode mode) :
        trimMode(mode), pos(0) { }
    void parse(const QString &text);
    QString sourceCode() const { return srcCode; }
    QString includeCode() const { return incCode; }

private:
    bool posMatchWith(const QString &str, int offset = 0) const;
    void parsePercentTag();
    QPair<QString, QString> parseEndPercentTag();
    void skipWhiteSpacesAndNewLineCode();
    QString parseQuote();

    TrimMode trimMode;
    QString erbData;
    QString srcCode;
    QString incCode;
    int pos;
    QString startTag;
};

