/* Copyright (c) 2016-2019, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "tjsloader.h"
#include "tsystemglobal.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <TWebApplication>

// #define tSystemError(fmt, ...)  printf(fmt "\n", ## __VA_ARGS__)
// #define tSystemDebug(fmt, ...)  printf(fmt "\n", ## __VA_ARGS__)

/*!
  \class TJSLoader
  \brief The TJSLoader class loads a JavaScript module at run-time.
*/

namespace {
QMap<QString, TJSModule *> jsContexts;
QStringList defaultPaths;
#if QT_VERSION < 0x060000
QMutex gMutex(QMutex::Recursive);
#else
QRecursiveMutex gMutex;
#endif
}


class SuffixMap : public QMap<int, QString> {
public:
    SuffixMap() :
        QMap<int, QString>()
    {
        insert(TJSLoader::Default, "js");
        insert(TJSLoader::Jsx, "jsx");
    }
};
Q_GLOBAL_STATIC(SuffixMap, suffixMap)


static QString read(const QString &filePath)
{
    QFile script(filePath);

    if (filePath.isEmpty()) {
        tSystemError("TJSLoader invalid file path");
        return QString();
    }

    if (!script.exists()) {
        tSystemError("TJSLoader file not found: %s", qUtf8Printable(filePath));
        return QString();
    }

    if (!script.open(QIODevice::ReadOnly)) {
        // open error
        tSystemError("TJSLoader file open error: %s", qUtf8Printable(filePath));
        return QString();
    }

    QTextStream stream(&script);
    QString program = stream.readAll();
    script.close();
    tSystemDebug("TJSLoader file read: %s", qUtf8Printable(script.fileName()));
    return program;
}


static bool isCommentPosition(const QString &content, int pos)
{
    if (pos < 0 || pos >= content.length()) {
        return false;
    }

    int idx = 0;
    while (idx < pos) {
        QChar ch = content[idx++];
        if (ch == '/') {
            auto str = content.mid(idx - 1, 2);
            if (str == "/*") {
                if ((idx = content.indexOf("*/", idx)) < 0) {
                    break;
                }
                idx += 2;
            } else if (str == "//") {
                if ((idx = content.indexOf(QChar::LineFeed, idx)) < 0) {
                    break;
                }
                idx++;
            }
        } else if (ch == '\"') {
            if ((idx = content.indexOf("\"", idx)) < 0) {
                break;
            }
            idx++;

        } else if (ch == '\'') {
            if ((idx = content.indexOf("'", idx)) < 0) {
                break;
            }
            idx++;
        }
    }
    return (idx != pos);
}

/*!
  Constructor.
 */
TJSLoader::TJSLoader(const QString &moduleName, AltJS alt) :
    module(moduleName), altJs(alt), member(), searchPaths(defaultPaths)
{
}

/*!
  Constructor.
 */
TJSLoader::TJSLoader(const QString &defaultMember, const QString &moduleName, AltJS alt) :
    module(moduleName), altJs(alt), member(defaultMember), searchPaths(defaultPaths)
{
}

/*!
  Loads the JavaScript module and returns the JavaScript context
  if successful; otherwise returns null pointer.
  i.e. var \a defaultMember = require( \a moduleName );
*/
TJSModule *TJSLoader::load(bool reload)
{
    if (module.isEmpty()) {
        return nullptr;
    }

    QMutexLocker lock(&gMutex);
    QString key = member + QLatin1Char(';') + module;
    TJSModule *context = jsContexts.value(key);

    if (reload && context) {
        jsContexts.remove(key);
        context->deleteLater();
        context = nullptr;
    }

    if (!context) {
        context = new TJSModule();

        for (auto &p : (const QList<QPair<QString, QString>> &)importFiles) {
            // Imports as JavaScript
            TJSLoader(p.first, p.second, Default).importTo(context, false);
        }

        QJSValue res = importTo(context, true);
        if (res.isError()) {
            context->deleteLater();
            context = nullptr;
        } else {
            jsContexts.insert(key, context);
        }
    }
    return context;
}


QString TJSLoader::search(const QString &moduleName, AltJS alt) const
{
    QString filePath;

    for (const auto &spath : searchPaths) {
        QString p = spath.trimmed();
        if (p.isEmpty()) {
            continue;
        }

        QDir path(p);
        if (!path.isAbsolute()) {
            path = QDir(Tf::app()->webRootPath() + p);
        }

        const QString suffix = QChar('.') + suffixMap()->value(alt);
        QString mod = (moduleName.endsWith(suffix)) ? moduleName : (moduleName + suffix);
        QString fpath = path.absoluteFilePath(mod);
        if (QFileInfo(fpath).exists()) {
            filePath = fpath;
            break;
        }

        // search package.json
        path = QDir(path.filePath(moduleName));
        QFile packageJson(path.filePath("package.json"));
        if (packageJson.exists() && packageJson.open(QIODevice::ReadOnly)) {
            auto json = QJsonDocument::fromJson(packageJson.readAll()).object();
            auto mainjs = json.value("main").toString();
            if (!mainjs.isEmpty()) {
                filePath = path.absoluteFilePath(mainjs);
                break;
            }
        }
    }
    return QFileInfo(filePath).canonicalFilePath();
}


QString TJSLoader::absolutePath(const QString &moduleName, const QDir &dir, AltJS alt) const
{
    QString filePath;

    if (moduleName.isEmpty()) {
        return filePath;
    }

    QFileInfo fi(moduleName);
    const QString suffix = QChar('.') + suffixMap()->value(alt);

    if (fi.isAbsolute()) {
        filePath = (fi.suffix().toLower() == suffixMap()->value(altJs)) ? moduleName : (moduleName + suffix);
    } else if (moduleName.startsWith("./")) {
        QString mod = (fi.suffix().toLower() == suffixMap()->value(altJs)) ? moduleName : (moduleName + suffix);
        filePath = dir.absoluteFilePath(mod);
    } else if (dir.exists(moduleName + suffix)) {
        filePath = dir.absoluteFilePath(moduleName + suffix);
    } else {
        filePath = search(moduleName, alt);
    }

    if (filePath.isEmpty()) {
        tSystemError("TJSLoader file not found: %s", qUtf8Printable(moduleName));
    } else {
        filePath = QFileInfo(filePath).canonicalFilePath();
    }
    return filePath;
}


void TJSLoader::import(const QString &moduleName)
{
    import(QString(), moduleName);
}


void TJSLoader::import(const QString &defaultMember, const QString &moduleName)
{
    for (auto &p : (const QList<QPair<QString, QString>> &)importFiles) {
        if (p.first == defaultMember && p.second == moduleName) {
            return;
        }
    }
    importFiles << qMakePair(defaultMember, moduleName);
}


QJSValue TJSLoader::importTo(TJSModule *context, bool isMain) const
{
    if (!context) {
        return false;
    }

    QJSValue ret;
    QString program;
    QString filePath;

    if (member.isEmpty()) {
        // loads module
        filePath = search(module, altJs);
        if (filePath.isEmpty()) {
            return false;
        }

        program = read(filePath);
        if (altJs == Jsx) {
            // Compiles JSX
            program = compileJsx(program);
        }
        replaceRequire(context, program, QFileInfo(filePath).dir());

    } else {
        // requires module
        program = QString("var %1 = require('%2');").arg(member).arg(module);
        replaceRequire(context, program, QDir("."));
        filePath = absolutePath(module, QDir("."), altJs);
    }

    if (program.isEmpty()) {
        return false;
    }

    ret = context->evaluate(program, module);
    if (!ret.isError()) {
        tSystemDebug("TJSLoader evaluation completed: %s", qUtf8Printable(module));

        if (isMain) {
            context->moduleFilePath = filePath;
            tSystemDebug("TJSLoader Module path: %s", qUtf8Printable(context->moduleFilePath));
        }
    }
    return ret;
}


TJSInstance TJSLoader::loadAsConstructor(const QJSValue &arg) const
{
    QJSValueList args = {arg};
    return loadAsConstructor(args);
}


TJSInstance TJSLoader::loadAsConstructor(const QJSValueList &args) const
{
    QMutexLocker lock(&gMutex);
    QString constructorName = (member.isEmpty()) ? QLatin1String("_TF_") + QFileInfo(module).baseName().replace(QChar('-'), QChar('_')) : member;
    auto *ctx = TJSLoader(constructorName, module).load();
    return (ctx) ? ctx->callAsConstructor(constructorName, args) : TJSInstance();
}


void TJSLoader::setSearchPaths(const QStringList &paths)
{
    searchPaths = paths + searchPaths;
}


void TJSLoader::setDefaultSearchPaths(const QStringList &paths)
{
    QMutexLocker lock(&gMutex);
    defaultPaths = paths;
}


QStringList TJSLoader::defaultSearchPaths()
{
    QMutexLocker lock(&gMutex);
    return defaultPaths;
}


void TJSLoader::replaceRequire(TJSModule *context, QString &content, const QDir &dir) const
{
    const QRegularExpression rx("require\\s*\\(\\s*[\"']([^\\(\\)\"' ,]+)[\"']\\s*\\)");

    if (!context || content.isEmpty()) {
        return;
    }

    int pos = 0;
    auto crc = content.toLatin1();
#if QT_VERSION < 0x060000
    const QString varprefix = QLatin1String("_tf%1_") + QString::number(qChecksum(crc.data(), crc.length()), 36) + "_%2";
#else
    const QString varprefix = QLatin1String("_tf%1_") + QString::number(qChecksum(crc), 36) + "_%2";
#endif

    for (;;) {
        auto match = rx.match(content, pos);
        if (!match.hasMatch()) {
            break;
        }

        pos = match.capturedStart();
        if (isCommentPosition(content, pos)) {
            pos += match.capturedLength();
            continue;
        }

        QString varName;
        auto module = match.captured(1);
        auto filePath = absolutePath(module, dir, Default);

        if (!module.isEmpty() && !filePath.isEmpty()) {
            // Check if it's loaded file
            varName = context->loadedFiles.value(filePath);

            if (varName.isEmpty()) {
                auto require = read(filePath);
                // replaces 'require'
                replaceRequire(context, require, QFileInfo(filePath).dir());
                if (require.isEmpty()) {
                    continue;
                }

                varName = varprefix.arg(QString::number(Tf::rand32_r(), 36)).arg(QString::number(pos, 36));
                require.prepend(QString("var %1=function(){").arg(varName));
                require.append(";return module.exports;}();");

                QJSValue res = context->evaluate(require, module);
                if (res.isError()) {
                    tSystemError("TJSLoader evaluation error: %s", qUtf8Printable(module));
                } else {
                    tSystemDebug("TJSLoader evaluation completed: %s", qUtf8Printable(module));
                    // Inserts the loaded file path
                    context->loadedFiles.insert(filePath, varName);
                }
            }

            content.replace(pos, match.capturedLength(), varName);
            pos += varName.length();
        } else {
            pos++;
        }
    }
}


QString TJSLoader::compileJsx(const QString &jsx)
{
    auto *transform = TJSLoader("JSXTransformer", "JSXTransformer").load();
    QJSValue jscode = transform->call("JSXTransformer.transform", QJSValue(jsx));
    //tSystemDebug("code:%s", qUtf8Printable(jscode.property("code").toString()));
    return jscode.property("code").toString();
}
