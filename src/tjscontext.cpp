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
#include "tjscontext.h"
#include "tsystemglobal.h"

static QStringList importPaths = { "." };


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
    : jsEngine(new QJSEngine()), commonJs(commonJsMode), funcObj(nullptr),
      lastFunc(), mutex(QMutex::Recursive)
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


QString TJSContext::read(const QString &moduleName, const QDir &dir)
{
    QString fileName;

    if (moduleName.startsWith("./")) {
        fileName = dir.filePath(moduleName);
    } else {
        for (auto &path : importPaths) {
            QString mod = moduleName;
            if (QFileInfo(mod).suffix().isEmpty()) {
                mod += ".*";
            }

            for (auto &f : QDir(path).entryList(QStringList(mod), QDir::Files)) {
                if (QFileInfo(f).suffix() == "js") {
                    fileName = f;
                    break;
                }
            }
        }
    }

    if (fileName.isEmpty()) {
        tSystemError("TJSContext file not found: %s\n", qPrintable(moduleName));
        return QString();
    }

    QFile script(fileName);
    if (!script.open(QIODevice::ReadOnly)) {
        // open error
        tSystemError("TJSContext file open error: %s", qPrintable(fileName));
        return QString();
    }

    QTextStream stream(&script);
    QString program = stream.readAll();
    script.close();

    if (commonJs) {
        QDir dir = QFileInfo(fileName).dir();
        replaceRequire(program, dir);


    }
    return program;
}


bool TJSContext::load(const QString &moduleName, const QDir &dir)
{
    auto program = read(moduleName, dir);

    QMutexLocker locker(&mutex);
    QJSValue res = evaluate(program, moduleName);
    if (res.isError()) {
        return false;
    }

    tSystemDebug("TJSContext evaluation completed: %s", qPrintable(moduleName));
    return true;
}


void TJSContext::replaceRequire(QString &content, const QDir &dir)
{
    const QRegExp rx("require\\s*\\(\\s*[\"']([^\\(\\)\"' ]+)[\"']\\s*\\)");

    int pos = 0;
    QString prefix = QLatin1String("_tf") + QString::number(Tf::rand32_r()) + "_";

    while ((pos = rx.indexIn(content, pos)) != -1) {
        if (isCommentPosition(content, pos)) {
            pos += rx.matchedLength();
            continue;
        }

        auto module = rx.cap(1);
        QString require;
        if (!module.isEmpty()) {
            require = read(module, dir);
        }

        QString var = prefix + QString::number(pos);
        if (commonJs) {
            require.prepend(QString("var %1=function(){").arg(var));
            require.append(";return module.exports;}();");
        }

        QMutexLocker locker(&mutex);
        QJSValue res = evaluate(require, module);
        if (res.isError()) {
            tSystemError("TJSContext evaluation error: %s", qPrintable(module));
        } else {
            tSystemDebug("TJSContext evaluation completed: %s", qPrintable(module));
        }

        content.replace(pos, rx.matchedLength(), var);
        pos += var.length();
    }
}
