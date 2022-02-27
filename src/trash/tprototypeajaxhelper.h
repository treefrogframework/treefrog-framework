#pragma once
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <TGlobal>
#include <THtmlAttribute>
#include <TJavaScriptObject>
#include <TOption>

class TActionView;


class T_CORE_EXPORT TPrototypeAjaxHelper {
public:
    enum UpdateBehavior {
        Replace = 0,  // Replace the element
        InsertBefore,  // Insert before the element
        InsertAfter,  // Insert after the element
        InsertAtTopOfContent,  // Insert at the top of the content of the element
        InsertAtBottomOfContent,  // Insert at the bottom of the content of the element
    };

    virtual ~TPrototypeAjaxHelper() { }

    QString requestFunction(const QUrl &url, const TOption &options,
        const QString &jsCondition = QString()) const;

    QString updateFunction(const QUrl &url, const QString &id,
        UpdateBehavior behavior = Replace,
        const TOption &options = TOption(),
        bool evalScripts = false,
        const QString &jsCondition = QString()) const;

    QString periodicalUpdateFunction(const QUrl &url, const QString &id,
        UpdateBehavior behavior = Replace,
        const TOption &options = TOption(),
        bool evalScripts = false,
        int frequency = 2, int decay = 1,
        const QString &jsCondition = QString()) const;

    QString linkToRequest(const QString &text, const QUrl &url, const TOption &options,
        const QString &jsCondition = QString(),
        const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString linkToUpdate(const QString &text, const QUrl &url, const QString &id,
        UpdateBehavior behavior = Replace,
        const TOption &options = TOption(),
        bool evalScripts = false, const QString &jsCondition = QString(),
        const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString linkToPeriodicalUpdate(const QString &text, const QUrl &url, const QString &id,
        UpdateBehavior behavior = Replace,
        const TOption &options = TOption(),
        bool evalScripts = false, int frequency = 2,
        int decay = 1, const QString &jsCondition = QString(),
        const THtmlAttribute &attributes = THtmlAttribute()) const;

    TOption o(int key, const QVariant &value) const;
    TOption o() const { return TOption(); }
    QVariantMap o(const QString &key, const TJavaScriptObject &value) const;
    QVariantMap o(const QString &key, const QVariant &value) const;

protected:
    virtual const TActionView *actionView() const = 0;
    QString optionsToString(const TOption &options) const;
};


inline TOption TPrototypeAjaxHelper::o(int key, const QVariant &value) const
{
    TOption option;
    option.insert(key, value);
    return option;
}

inline QVariantMap TPrototypeAjaxHelper::o(const QString &key, const TJavaScriptObject &value) const
{
    QVariantMap map;
    QVariant v;
    v.setValue(value);
    map.insert(key, v);
    return map;
}

inline QVariantMap TPrototypeAjaxHelper::o(const QString &key, const QVariant &value) const
{
    QVariantMap map;
    map.insert(key, value);
    return map;
}

