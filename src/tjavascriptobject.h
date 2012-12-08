#ifndef TJAVASCRIPTOBJECT_H
#define TJAVASCRIPTOBJECT_H

#include <QLatin1String>


class TJavaScriptObject : public QLatin1String
{
public:
    explicit TJavaScriptObject(const char *str = "");
    TJavaScriptObject(const TJavaScriptObject &other);
    QString toString() const;
};


inline TJavaScriptObject::TJavaScriptObject(const char *str)
    : QLatin1String(str)
{ }

inline TJavaScriptObject::TJavaScriptObject(const TJavaScriptObject &other)
    : QLatin1String(other)
{ }

inline QString TJavaScriptObject::toString() const
{
    return QString(latin1());
}

Q_DECLARE_METATYPE(TJavaScriptObject)

#endif // TJAVASCRIPTOBJECT_H
