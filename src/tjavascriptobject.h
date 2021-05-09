#pragma once
#include <QLatin1String>


class TJavaScriptObject : public QLatin1String {
public:
    explicit TJavaScriptObject(const char *str = "");
    TJavaScriptObject(const TJavaScriptObject &other) = default;
    QString toString() const;
};


inline TJavaScriptObject::TJavaScriptObject(const char *str) :
    QLatin1String(str)
{
}


inline QString TJavaScriptObject::toString() const
{
    return QString(latin1());
}

Q_DECLARE_METATYPE(TJavaScriptObject)

