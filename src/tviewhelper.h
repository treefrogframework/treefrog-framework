#ifndef TVIEWHELPER_H
#define TVIEWHELPER_H

#include <QStringList>
#include <QVariant>
#include <QUrl>
#include <QPoint>
#include <QSize>
#include <TGlobal>
#include <THtmlAttribute>

class TActionView;


class T_CORE_EXPORT TViewHelper
{
public:
    virtual ~TViewHelper() { }

    QString linkTo(const QString &text, const QUrl &url, Tf::HttpMethod method = Tf::Get,
                   const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString linkTo(const QString &text, const QUrl &url, Tf::HttpMethod method,
                   const QString &jsCondition, const THtmlAttribute &attributes = THtmlAttribute()) const;
    
    QString linkToPopup(const QString &text, const QUrl &url,
                        const QString &windowTitle = QString(),
                        const QSize &size = QSize(), const QPoint &topLeft = QPoint(),
                        const QString &windowStyle = QString(),
                        const QString &jsCondition = QString(),
                        const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString linkToIf(bool condition, const QString &text, const QUrl &url,
                     Tf::HttpMethod method = Tf::Get, const QString &jsCondition = QString(),
                     const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString linkToUnless(bool condition, const QString &text, const QUrl &url,
                         Tf::HttpMethod method = Tf::Get, const QString &jsCondition = QString(),
                         const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString linkToFunction(const QString &text, const QString &function,
                           const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString buttonToFunction(const QString &text, const QString &function,
                             const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString anchor(const QString &text, const QUrl &url, Tf::HttpMethod method = Tf::Get,
                   const QString &jsCondition = QString(),
                   const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString anchorPopup(const QString &text, const QUrl &url,
                        const QString &windowTitle = QString(),
                        const QSize &size = QSize(), const QPoint &topLeft = QPoint(),
                        const QString &windowStyle = QString(),
                        const QString &jsCondition = QString(),
                        const THtmlAttribute &attributes = THtmlAttribute()) const;
    
    QString anchorIf(bool condition, const QString &text, const QUrl &url,
                     Tf::HttpMethod method = Tf::Get, const QString &jsCondition = QString(),
                     const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString anchorUnless(bool condition, const QString &text, const QUrl &url,
                         Tf::HttpMethod method = Tf::Get, const QString &jsCondition = QString(),
                         const THtmlAttribute &attributes = THtmlAttribute()) const;
    
    QString anchorFunction(const QString &text, const QString &function,
                             const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString formTag(const QUrl &url, Tf::HttpMethod method = Tf::Post, bool multipart = false,
                    const THtmlAttribute &attributes = THtmlAttribute());
    
    QString endTag();

    QString allEndTags();
    
    QString inputTag(const QString &type, const QString &name, const QVariant &value,
                     const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString inputTextTag(const QString &name, const QVariant &value,
                         const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString inputFileTag(const QString &name, const QVariant &value,
                         const THtmlAttribute &attributes = THtmlAttribute()) const;
    
    QString inputPasswordTag(const QString &name, const QVariant &value,
                             const THtmlAttribute &attributes = THtmlAttribute()) const;
    
    QString inputHiddenTag(const QString &name, const QVariant &value,
                           const THtmlAttribute &attributes = THtmlAttribute()) const;
    
    QString checkBoxTag(const QString &name, const QString &value, bool checked = false,
                        const THtmlAttribute &attributes = THtmlAttribute()) const;
    
    QString radioButtonTag(const QString &name, const QString &value, bool checked = false,
                           const THtmlAttribute &attributes = THtmlAttribute()) const;
    
    QString inputAuthenticityTag() const;
    
    QString textAreaTag(const QString &name, int rows, int cols, const QString &content = QString(),
                        const THtmlAttribute &attributes = THtmlAttribute()) const;
    
    QString submitTag(const QString &value, const THtmlAttribute &attributes = THtmlAttribute()) const;
    
    QString submitImageTag(const QString &src, const THtmlAttribute &attributes = THtmlAttribute()) const;
    
    QString resetTag(const QString &value, const THtmlAttribute &attributes = THtmlAttribute()) const;
    
    QString imageTag(const QString &src, const QSize &size = QSize(),
                     const QString &alt = QString(),
                     const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString imageTag(const QString &src, bool withTimestamp,
                     const QSize &size = QSize(), const QString &alt = QString(),
                     const THtmlAttribute &attributes = THtmlAttribute()) const;

    QString imageLinkTo(const QString &src, const QUrl &url, const QSize &size = QSize(),
                        const QString &alt = QString(), const THtmlAttribute &attributes = THtmlAttribute()) const;
    
    QString stylesheetTag(const QString &src, const THtmlAttribute &attributes = THtmlAttribute()) const;

    THtmlAttribute a(const QString &key, const QString &value) const;
    THtmlAttribute a() const { return THtmlAttribute(); }
    
protected:
    virtual const TActionView *actionView() const = 0;
    QString tag(const QString &name, const THtmlAttribute &attributes, bool selfClosing = true) const;
    QString tag(const QString &name, const THtmlAttribute &attributes, const QString &content) const;
    QString imagePath(const QString &src, bool withTimestamp = false) const;
    QString cssPath(const QString &src) const;
    QString jsPath(const QString &src) const;
    QString srcPath(const QString &src, const QString &dir, bool withTimestamp = false) const;

private:
    QStringList endTags;
};


inline QString TViewHelper::linkToIf(bool condition, const QString &text, const QUrl &url, Tf::HttpMethod method,
                                     const QString &jsCondition, const THtmlAttribute &attributes) const
{
    return condition ? linkTo(text, url, method, jsCondition, attributes) : QString();
}


inline QString TViewHelper::linkToUnless(bool condition, const QString &text, const QUrl &url, Tf::HttpMethod method,
                                         const QString &jsCondition, const THtmlAttribute &attributes) const
{
    return linkToIf(!condition, text, url, method, jsCondition, attributes);
}

inline QString TViewHelper::anchor(const QString &text, const QUrl &url, Tf::HttpMethod method,
                                   const QString &jsCondition, const THtmlAttribute &attributes) const
{
    return linkTo(text, url, method, jsCondition, attributes);
}

inline QString TViewHelper::anchorPopup(const QString &text, const QUrl &url,
                                        const QString &windowTitle,
                                        const QSize &size, const QPoint &topLeft,
                                        const QString &windowStyle,
                                        const QString &jsCondition,
                                        const THtmlAttribute &attributes) const
{
    return linkToPopup(text, url, windowTitle, size, topLeft, windowStyle, jsCondition, attributes);
}

inline QString TViewHelper::anchorIf(bool condition, const QString &text,
                                     const QUrl &url, Tf::HttpMethod method,
                                     const QString &jsCondition,
                                     const THtmlAttribute &attributes) const
{
    return linkToIf(condition, text, url, method, jsCondition, attributes);
}

inline QString TViewHelper::anchorUnless(bool condition, const QString &text,
                                         const QUrl &url, Tf::HttpMethod method,
                                         const QString &jsCondition,
                                         const THtmlAttribute &attributes) const
{
    return linkToUnless(condition, text, url, method, jsCondition, attributes);
}
    
inline QString TViewHelper::anchorFunction(const QString &text,
                                           const QString &function,
                                           const THtmlAttribute &attributes) const
{
    return linkToFunction(text, function, attributes);
}

inline QString TViewHelper::inputTextTag(const QString &name, const QVariant &value,
                                         const THtmlAttribute &attributes) const
{
    return inputTag("text", name, value, attributes);
}


inline QString TViewHelper::inputFileTag(const QString &name, const QVariant &value,
                                         const THtmlAttribute &attributes) const
{
    return inputTag("file", name, value, attributes);
}

    
inline QString TViewHelper::inputPasswordTag(const QString &name, const QVariant &value,
                                             const THtmlAttribute &attributes) const
{
    return inputTag("password", name, value, attributes);
}

    
inline QString TViewHelper::inputHiddenTag(const QString &name, const QVariant &value,
                                           const THtmlAttribute &attributes) const
{
    return inputTag("hidden", name, value, attributes);
}

inline QString TViewHelper::imageLinkTo(const QString &src, const QUrl &url,
                                        const QSize &size, const QString &alt,
                                        const THtmlAttribute &attributes) const
{
    return linkTo(imageTag(src, size, alt, attributes), url);
}

#endif // TVIEWHELPER_H
