#pragma once
#include <QJSValue>
#include <TGlobal>
#include <TJSInstance>
#include <TJSModule>


class T_CORE_EXPORT TJSLoader {
public:
    enum AltJS {
        Default = 0,  // JavaScript (ES5)
        Jsx,
    };

    TJSLoader(const QString &moduleName, AltJS alt = Default);
    TJSLoader(const QString &defaultMember, const QString &moduleName, AltJS alt = Default);

    TJSModule *load(bool reload = false);
    void import(const QString &moduleName);
    void import(const QString &defaultMember, const QString &moduleName);
    TJSInstance loadAsConstructor(const QJSValue &arg) const;
    TJSInstance loadAsConstructor(const QJSValueList &args = QJSValueList()) const;

    void setSearchPaths(const QStringList &paths);
    static void setDefaultSearchPaths(const QStringList &paths);
    static QStringList defaultSearchPaths();
    static QString compileJsx(const QString &jsx);

protected:
    QJSValue importTo(TJSModule *context, bool isMain) const;
    QString search(const QString &moduleName, AltJS alt) const;
    QString absolutePath(const QString &moduleName, const QDir &dir, AltJS alt) const;
    void replaceRequire(TJSModule *context, QString &content, const QDir &dir) const;

private:
    QString module;
    AltJS altJs;
    QString member;
    QStringList searchPaths;
    QList<QPair<QString, QString>> importFiles;

    friend class TJSModule;
};

