#pragma once
#include <QMultiMap>
#include <QStringList>


class OtmParser {
public:
    enum OperatorType {
        TagReplacement = 0,
        ContentAssignment,
        AttributeSet,
        TagMerging,
    };

    enum EchoOption {
        None = 0,
        NormalEcho,
        EscapeEcho,
        ExportVarEcho,
        ExportVarEscapeEcho,
    };

    OtmParser(const QString &replaceMarker);
    void parse(const QString &text);

    QString getSrcCode(const QString &label, OperatorType op, EchoOption *option = 0) const;
    QStringList getWrapSrcCode(const QString &label, OperatorType op) const;
    QStringList includeStrings() const;
    QString getInitSrcCode() const;

protected:
    void parse();

private:
    QMultiMap<QString, QString> entries;
    QString repMarker;
};

