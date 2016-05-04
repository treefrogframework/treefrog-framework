/* Copyright (c) 2016, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QJSEngine>
#include <QJSValue>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <TWebApplication>
#include "tjscontext.h"
#include "tsystemglobal.h"

static QStringList searchPaths = { "." };
//#define tSystemError(fmt, ...)  printf(fmt "\n", __VA_ARGS__)
//#define tSystemDebug(fmt, ...)  printf(fmt "\n", __VA_ARGS__)


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


TJSContext::TJSContext(bool commonJsMode, const QStringList &scriptFiles)
    : jsEngine(new QJSEngine()), commonJs(commonJsMode), loadedFiles(),
      funcObj(nullptr), lastFunc(), mutex(QMutex::Recursive)
{
    if (commonJsMode) {
        jsEngine->evaluate("exports={};module={};module.exports={};");
    }

    for (auto &file : scriptFiles) {
        load(file, QDir("."));
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


static QString absolutePath(const QString &moduleName, const QDir &dir)
{
    QString filePath;

    if (moduleName.isEmpty()) {
        return filePath;
    }

    if (QFileInfo(moduleName).isAbsolute()) {
        if (QFileInfo(moduleName + ".js").exists()) {
            filePath = moduleName + ".js";
        } else {
            filePath = moduleName;
        }

    } else if (moduleName.startsWith("./")) {
        if (dir.exists(moduleName + ".js")) {
            filePath = dir.absoluteFilePath(moduleName + ".js");
        } else {
            filePath = dir.absoluteFilePath(moduleName);
        }

    } else if (dir.exists(moduleName + ".js")) {
        filePath = dir.absoluteFilePath(moduleName + ".js");
    } else {
        for (auto &p : searchPaths) {
            QDir path(p);
            if (!path.isAbsolute()) {
                path = QDir(Tf::app()->webRootPath() + p);
            }

            QString fpath = path.absoluteFilePath(moduleName + ".js");
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
    }

    if (filePath.isEmpty()) {
        tSystemError("TJSContext file not found: %s\n", qPrintable(moduleName));
    } else {
        filePath = QFileInfo(filePath).canonicalFilePath();
        tSystemDebug("TJSContext search path: %s", qPrintable(filePath));
    }
    return filePath;
}


QString TJSContext::read(const QString &filePath)
{
    QMutexLocker locker(&mutex);
    QFile script(filePath);

    if (filePath.isEmpty() || !script.exists()) {
        tSystemError("TJSContext file not found: %s\n", qPrintable(filePath));
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

    if (commonJs) {
        replaceRequire(program, QFileInfo(filePath).dir());
    }
    return program;
}


QJSValue TJSContext::load(const QString &moduleName, const QDir &dir)
{
    auto filePath = absolutePath(moduleName, dir);
    auto program = read(filePath);

    QMutexLocker locker(&mutex);
    QJSValue ret = evaluate(program, moduleName);

    if (!ret.isError()) {
        tSystemDebug("TJSContext evaluation completed: %s", qPrintable(moduleName));
    }
    return ret;
}


void TJSContext::replaceRequire(QString &content, const QDir &dir)
{
    const QRegExp rx("require\\s*\\(\\s*[\"']([^\\(\\)\"' ]+)[\"']\\s*\\)");

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

                varName = varprefix.arg(QString::number(Tf::rand32_r(), 36)).arg(QString::number(pos, 36));
                if (commonJs) {
                    require.prepend(QString("var %1=function(){").arg(varName));
                    require.append(";return module.exports;}();");
                }

                QMutexLocker locker(&mutex);
                QJSValue res = evaluate(require, module);
                if (res.isError()) {
                    tSystemError("TJSContext evaluation error: %s", qPrintable(module));
                } else {
                    tSystemDebug("TJSContext evaluation completed: %s", qPrintable(module));
                    // Inserts the loaded file path
                    loadedFiles.insert(filePath, varName);
                }
            }
        }

        content.replace(pos, rx.matchedLength(), varName);
        pos += varName.length();
    }
}
