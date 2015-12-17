#ifndef THTMLATTRIBUTE_H
#define THTMLATTRIBUTE_H

#include <QStringList>
#include <QPair>
#include <TGlobal>


class T_CORE_EXPORT THtmlAttribute : public QList<QPair<QString, QString>>
{
public:
    THtmlAttribute() { }
    THtmlAttribute(const THtmlAttribute &other);
    THtmlAttribute(const QList<QPair<QString, QString>> &list);

    bool contains(const QString &key) const;
    void prepend(const QString &key, const QString &value);
    void append(const QString &key, const QString &value);
    THtmlAttribute &operator=(const THtmlAttribute &other);
    THtmlAttribute operator|(const THtmlAttribute &other) const;
    QString toString(bool escape = true) const;
};

Q_DECLARE_METATYPE(THtmlAttribute)

#endif // TVIEWHELPER_H
