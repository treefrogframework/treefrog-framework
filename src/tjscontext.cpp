/* Copyright (c) 2016, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QJSEngine>
#include <QJSValue>
#include <QJSValueIterator>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <TWebApplication>
#include <TJSContext>
#include <TJSInstance>
#include "tsystemglobal.h"

static QStringList searchPaths = { "." };
//#define tSystemError(fmt, ...)  printf(fmt "\n", ## __VA_ARGS__)
//#define tSystemDebug(fmt, ...)  printf(fmt "\n", ## __VA_ARGS__)


// static void dump(const QJSValue &object)
// {
//     QJSValueIterator it(object);
//     while (it.hasNext()) {
//         it.next();
//         tSystemDebug("%s: %s", qPrintable(it.name()), qPrintable(it.value().toString()));
//     }
// }


void TJSContext::setSearchPaths(const QStringList &paths)
{
    searchPaths = paths;
}


inline const char *prop(const QJSValue &val, const QString &name = QString())
{
    return (name.isEmpty()) ? qPrintable(val.toString()) : qPrintable(val.property(name).toString());
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


TJSContext::TJSContext(const QStringList &scriptFiles)
    : jsEngine(new QJSEngine()), loadedFiles(), funcObj(nullptr),
      lastFunc(), mutex(QMutex::Recursive)
{
    jsEngine->evaluate("exports={};module={};module.exports={};");

    for (auto &file : scriptFiles) {
        import(file);
    }
}


TJSContext::~TJSContext()
{
    if (funcObj) {
        delete funcObj;
    }
    delete jsEngine;
}


QJSValue TJSContext::evaluate(const QString &program, const QString &fileName, int lineNumber)
{
    QMutexLocker locker(&mutex);

    QJSValue ret = jsEngine->evaluate(program, fileName, lineNumber);
    if (ret.isError()) {
        tSystemError("JS uncaught exception at %s:%s : %s", prop(ret, "fileName"),
                     prop(ret, "lineNumber"), prop(ret, "message"));
    }
    return ret;
}


QJSValue TJSContext::call(const QString &func, const QJSValue &arg)
{
    QJSValueList args = { arg };
    return call(func, args);
}


QJSValue TJSContext::call(const QString &func, const QJSValueList &args)
{
    QMutexLocker locker(&mutex);
    QJSValue ret;

    QString funcsym = QString::number(args.count()) + func;
    if (funcsym != lastFunc || !funcObj) {
        lastFunc = funcsym;

        QString argstr;
        for (int i = 0; i < args.count(); i++) {
            argstr = QChar('a') + QString::number(i) + ',';
        }
        argstr.chop(1);

        QString defFunc = QString("function(%1){return(%2(%1));}").arg(argstr, func);

        if (!funcObj) {
            funcObj = new QJSValue();
        }

        *funcObj = evaluate(defFunc);
        if (funcObj->isError()) {
            goto eval_error;
        }
    }

    ret = funcObj->call(args);
    if (ret.isError()) {
        tSystemError("JS uncaught exception at %s:%s : %s", prop(ret, "fileName"),
                     prop(ret, "lineNumber"), prop(ret));
        goto eval_error;
    }

    return ret;

eval_error:
    delete funcObj;
    funcObj = nullptr;
    return ret;
}


TJSInstance TJSContext::callAsConstructor(const QString &className, const QJSValueList &args)
{
    QMutexLocker locker(&mutex);

    QJSValue construct = evaluate(className);
    tSystemDebug("construct: %s", qPrintable(construct.toString()));
    QJSValue res = construct.callAsConstructor(args);
    if (res.isError()) {
        tSystemError("JS uncaught exception at %s:%s : %s", prop(res, "fileName"),
                     prop(res, "lineNumber"), prop(res));
    }
    return TJSInstance(res);
}


static QString search(const QString &moduleName)
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

        QString mod = (moduleName.endsWith(".js")) ? moduleName : (moduleName + ".js");
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


static QString absolutePath(const QString &moduleName, const QDir &dir)
{
    QString filePath;

    if (moduleName.isEmpty()) {
        return filePath;
    }

    QFileInfo fi(moduleName);

    if (fi.isAbsolute()) {
        filePath = (fi.suffix() == "js") ? moduleName : (moduleName + ".js");
    } else if (moduleName.startsWith("./")) {
        QString mod = (fi.suffix() == "js") ? moduleName : (moduleName + ".js");
        filePath = dir.absoluteFilePath(mod);
    } else if (dir.exists(moduleName + ".js")) {
        filePath = dir.absoluteFilePath(moduleName + ".js");
    } else {
        filePath = search(moduleName);
    }

    if (filePath.isEmpty()) {
        tSystemError("TJSContext file not found: %s", qPrintable(moduleName));
    } else {
        filePath = QFileInfo(filePath).canonicalFilePath();
        tSystemDebug("TJSContext search path: %s", qPrintable(filePath));
    }
    return filePath;
}


QString TJSContext::read(const QString &filePath)
{
    QFile script(filePath);

    if (filePath.isEmpty()) {
        tSystemError("TJSContext invalid file path");
        return QString();
    }

    if (!script.exists()) {
        tSystemError("TJSContext file not found: %s", qPrintable(filePath));
        return QString();
    }

    if (!script.open(QIODevice::ReadOnly)) {
        // open error
        tSystemError("TJSContext file open error: %s", qPrintable(filePath));
        return QString();
    }

    QTextStream stream(&script);
    QString program = stream.readAll();
    script.close();

    replaceRequire(program, QFileInfo(filePath).dir());
    return program;
}


QJSValue TJSContext::import(const QString &defaultMember, const QString &moduleName)
{
    QJSValue ret;
    QString program = QString("var %1 = require('%2');").arg(defaultMember).arg(moduleName);
    replaceRequire(program, QDir("."));

    if (!program.isEmpty()) {
        ret = evaluate(program, moduleName);

        if (!ret.isError()) {
            tSystemDebug("TJSContext evaluation completed: %s", qPrintable(moduleName));
        }
    }
    return ret;
}


QJSValue TJSContext::import(const QString &moduleName)
{
    QJSValue ret;
    auto filePath = search(moduleName);
    if (filePath.isEmpty()) {
        return ret;
    }

    auto program = read(filePath);
    if (!program.isEmpty()) {
        ret = evaluate(program, moduleName);

        if (!ret.isError()) {
            tSystemDebug("TJSContext evaluation completed: %s", qPrintable(moduleName));
        }
    }
    return ret;
}


// QJSValue TJSContext::readModule(const QString &moduleName, const QDir &dir)
// {
//     QJSValue ret;
//     auto filePath = absolutePath(moduleName, dir);
//     auto program = read(filePath);

//     if (!program.isEmpty()) {
//         QMutexLocker locker(&mutex);
//         ret = evaluate(program, moduleName);

//         if (!ret.isError()) {
//             tSystemDebug("TJSContext evaluation completed: %s", qPrintable(moduleName));
//         }
//     }
//     return ret;
// }


void TJSContext::replaceRequire(QString &content, const QDir &dir)
{
    const QRegExp rx("require\\s*\\(\\s*[\"']([^\\(\\)\"' ,]+)[\"']\\s*\\)");

    int pos = 0;
    auto crc = content.toLatin1();
    const QString varprefix = QLatin1String("_tf%1_") + QString::number(qChecksum(crc.data(), crc.length()), 36) + "_%2";

    while ((pos = rx.indexIn(content, pos)) != -1) {
        if (isCommentPosition(content, pos)) {
            pos += rx.matchedLength();
            continue;
        }

        auto module = rx.cap(1);
        QString varName;
        auto filePath = absolutePath(module, dir);

        if (!module.isEmpty() && !filePath.isEmpty()) {
            // Check if it's loaded file
            varName = loadedFiles.value(filePath);

            if (varName.isEmpty()) {
                auto require = read(filePath);
                if (require.isEmpty()) {
                    continue;
                }

                varName = varprefix.arg(QString::number(Tf::rand32_r(), 36)).arg(QString::number(pos, 36));
                require.prepend(QString("var %1=function(){").arg(varName));
                require.append(";return module.exports;}();");

                QJSValue res = evaluate(require, module);
                if (res.isError()) {
                    tSystemError("TJSContext evaluation error: %s", qPrintable(module));
                } else {
                    tSystemDebug("TJSContext evaluation completed: %s", qPrintable(module));
                    // Inserts the loaded file path
                    loadedFiles.insert(filePath, varName);
                }
            }

            content.replace(pos, rx.matchedLength(), varName);
            pos += varName.length();
        } else {
            pos++;
        }
    }
}
