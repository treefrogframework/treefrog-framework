/* Copyright (c) 2010-2021, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "controllergenerator.h"
#include "filewriter.h"
#include "global.h"
#include "projectfilegenerator.h"
#include "tableschema.h"
#include <tfnamespace.h>
#include <QtCore>

constexpr auto CONTROLLER_HEADER_FILE_TEMPLATE = "#pragma once\n"
                                                 "#include \"applicationcontroller.h\"\n"
                                                 "\n\n"
                                                 "class T_CONTROLLER_EXPORT %clsname%Controller : public ApplicationController {\n"
                                                 "    Q_OBJECT\n"
                                                 "public slots:\n"
                                                 "    void index();\n"
                                                 "    void show(const QString &%id%);\n"
                                                 "    void create();\n"
                                                 "    void save(const QString &%id%);\n"
                                                 "    void remove(const QString &%id%);\n"
                                                 "};\n"
                                                 "\n";


constexpr auto CONTROLLER_SOURCE_FILE_TEMPLATE = "#include \"%name%controller.h\"\n"
                                                 "#include \"%name%service.h\"\n"
                                                 "#include <TreeFrogController>\n"
                                                 "\n"
                                                 "static %clsname%Service service;\n"
                                                 "\n\n"
                                                 "void %clsname%Controller::index()\n"
                                                 "{\n"
                                                 "    service.index();\n"
                                                 "    render();\n"
                                                 "}\n"
                                                 "\n"
                                                 "void %clsname%Controller::show(const QString &%id%)\n"
                                                 "{\n"
                                                 "    service.show(%var1%);\n"
                                                 "    render();\n"
                                                 "}\n"
                                                 "\n"
                                                 "void %clsname%Controller::create()\n"
                                                 "{\n"
                                                 "    %type% %id%;\n"
                                                 "\n"
                                                 "    switch (request().method()) {\n"
                                                 "    case Tf::Get:\n"
                                                 "        render();\n"
                                                 "        break;\n"
                                                 "    case Tf::Post:\n"
                                                 "        %id% = service.create(request());\n"
                                                 "        if (%condition%) {\n"
                                                 "            redirect(urla(\"show\", %id%));\n"
                                                 "        } else {\n"
                                                 "            render();\n"
                                                 "        }\n"
                                                 "        break;\n"
                                                 "    default:\n"
                                                 "        renderErrorResponse(Tf::NotFound);\n"
                                                 "        break;\n"
                                                 "    }\n"
                                                 "}\n"
                                                 "\n"
                                                 "void %clsname%Controller::save(const QString &%id%)\n"
                                                 "{\n"
                                                 "    int res;\n"
                                                 "\n"
                                                 "    switch (request().method()) {\n"
                                                 "    case Tf::Get:\n"
                                                 "        service.edit(session(), %var1%);\n"
                                                 "        render();\n"
                                                 "        break;\n"
                                                 "    case Tf::Post:\n"
                                                 "        res = service.save(request(), session(), %var1%);\n"
                                                 "        if (res > 0) {\n"
                                                 "            // Save completed\n"
                                                 "            redirect(urla(\"show\", %id%));\n"
                                                 "        } else if (res < 0) {\n"
                                                 "            // Failed\n"
                                                 "            render();\n"
                                                 "        } else {\n"
                                                 "            // Retry\n"
                                                 "            redirect(urla(\"save\", %id%));\n"
                                                 "        }\n"
                                                 "        break;\n"
                                                 "    default:\n"
                                                 "        renderErrorResponse(Tf::NotFound);\n"
                                                 "        break;\n"
                                                 "    }\n"
                                                 "}\n"
                                                 "\n"
                                                 "void %clsname%Controller::remove(const QString &%id%)\n"
                                                 "{\n"
                                                 "    switch (request().method()) {\n"
                                                 "    case Tf::Post:\n"
                                                 "        service.remove(%var1%);\n"
                                                 "        redirect(urla(\"index\"));\n"
                                                 "        break;\n"
                                                 "    default:\n"
                                                 "        renderErrorResponse(Tf::NotFound);\n"
                                                 "        break;\n"
                                                 "    }\n"
                                                 "}\n"
                                                 "\n"
                                                 "// Don't remove below this line\n"
                                                 "T_DEFINE_CONTROLLER(%clsname%Controller)\n";


constexpr auto CONTROLLER_TINY_HEADER_FILE_TEMPLATE = "#pragma once\n"
                                                      "\n"
                                                      "#include \"applicationcontroller.h\"\n"
                                                      "\n\n"
                                                      "class T_CONTROLLER_EXPORT %1Controller : public ApplicationController\n"
                                                      "{\n"
                                                      "    Q_OBJECT\n"
                                                      "public:\n"
                                                      "    %1Controller() : ApplicationController() { }\n"
                                                      "\n"
                                                      "public slots:\n"
                                                      "%2"
                                                      "};\n"
                                                      "\n";


constexpr auto CONTROLLER_TINY_SOURCE_FILE_TEMPLATE = "#include \"%1controller.h\"\n"
                                                      "\n\n"
                                                      "%3"
                                                      "// Don't remove below this line\n"
                                                      "T_DEFINE_CONTROLLER(%2Controller)\n";


class ConvMethod : public QHash<int, QString> {
public:
    ConvMethod() :
        QHash<int, QString>()
    {
        insert(QMetaType::Int, "%1.toInt()");
        insert(QMetaType::UInt, "%1.toUInt()");
        insert(QMetaType::LongLong, "%1.toLongLong()");
        insert(QMetaType::ULongLong, "%1.toULongLong()");
        insert(QMetaType::Double, "%1.toDouble()");
        insert(QMetaType::QByteArray, "%1.toByteArray()");
        insert(QMetaType::QString, "%1");
        insert(QMetaType::QDate, "QDate::fromString(%1)");
        insert(QMetaType::QTime, "QTime::fromString(%1)");
        insert(QMetaType::QDateTime, "QDateTime::fromString(%1)");
    }
};
Q_GLOBAL_STATIC(ConvMethod, convMethod)

class ConditionString : public QHash<int, QString> {
public:
    ConditionString() :
        QHash<int, QString>()
    {
        insert(QMetaType::Int, "%1 > 0");
        insert(QMetaType::UInt, "%1 > 0");
        insert(QMetaType::LongLong, "%1 > 0");
        insert(QMetaType::ULongLong, "%1 > 0");
        insert(QMetaType::Double, "%1 > 0");
        insert(QMetaType::QByteArray, "!%1.isEmpty()");
        insert(QMetaType::QString, "!%1.isEmpty()");
        insert(QMetaType::QDate, "!%1.isNull()");
        insert(QMetaType::QTime, "!%1.isNull()");
        insert(QMetaType::QDateTime, "!%1.isNull()");
    }
};
Q_GLOBAL_STATIC(ConditionString, conditionString)

class NGCtlrName : public QStringList {
public:
    NGCtlrName() :
        QStringList()
    {
        append("layouts");
        append("partial");
        append("direct");
        append("_src");
        append("mailer");
    }
};
Q_GLOBAL_STATIC(NGCtlrName, ngCtlrName)


ControllerGenerator::ControllerGenerator(const QString &controller, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int lockRevIdx) :
    controllerName(controller), fieldList(fields), primaryKeyIndex(pkIdx), lockRevIndex(lockRevIdx)
{
}


ControllerGenerator::ControllerGenerator(const QString &controller, const QStringList &actions) :
    controllerName(fieldNameToEnumName(controller)), actionList(actions)
{
}


bool ControllerGenerator::generate(const QString &dstDir) const
{
    // Reserved word check
    if (ngCtlrName()->contains(tableName.toLower())) {
        qCritical("Reserved word error. Please use another word.  Controller name: %s", qUtf8Printable(tableName));
        return false;
    }

    QDir dir(dstDir);
    QStringList files;
    FileWriter fwh(dir.filePath(controllerName.toLower() + "controller.h"));
    FileWriter fws(dir.filePath(controllerName.toLower() + "controller.cpp"));

    if (actionList.isEmpty()) {
        if (fieldList.isEmpty()) {
            qCritical("Incorrect parameters.");
            return false;
        }

        QPair<QString, QMetaType::Type> pair;
        if (primaryKeyIndex >= 0)
            pair = fieldList[primaryKeyIndex];

        // Generates a controller source code
        QString sessInsertStr;
        QString sessGetStr;
        QString revStr;
        QString varName = enumNameToVariableName(controllerName);

        if (lockRevIndex >= 0) {
            sessInsertStr = QString("            session().insert(\"%1_lockRevision\", model.lockRevision());\n").arg(varName);
            sessGetStr = QString("        int rev = session().value(\"%1_lockRevision\").toInt();\n").arg(varName);
            revStr = QLatin1String(", rev");
        }

        PlaceholderList replaceList = {
            {"name", controllerName.toLower()},
            {"clsname", controllerName},
            {"varname", varName},
            {"var1", convMethod()->value(pair.second).arg(fieldNameToVariableName(pair.first))},
            {"code1", sessInsertStr},
            {"code2", sessGetStr},
            {"type", QString::fromLatin1(QMetaType::typeName(pair.second))},
            {"rev", revStr},
            {"id", fieldNameToVariableName(pair.first)},
            {"condition", conditionString()->value(pair.second).arg(fieldNameToVariableName(pair.first))},
        };

        // Generates a controller header file
        QString code = replaceholder(CONTROLLER_HEADER_FILE_TEMPLATE, replaceList);
        fwh.write(code, false);
        files << fwh.fileName();

        // Generates a controller source file
        code = replaceholder(CONTROLLER_SOURCE_FILE_TEMPLATE, replaceList);
        fws.write(code, false);
        files << fws.fileName();

    } else {
        // Generates a controller header file
        QString actions;
        for (QStringListIterator i(actionList); i.hasNext();) {
            actions.append("    void ").append(i.next()).append("();\n");
        }

        QString code = QString(CONTROLLER_TINY_HEADER_FILE_TEMPLATE).arg(controllerName, actions);
        fwh.write(code, false);
        files << fwh.fileName();

        // Generates a controller source code
        QString actimpl;
        for (QStringListIterator i(actionList); i.hasNext();) {
            actimpl.append("void ").append(controllerName).append("Controller::").append(i.next()).append("()\n{\n    // write code\n}\n\n");
        }
        code = QString(CONTROLLER_TINY_SOURCE_FILE_TEMPLATE).arg(controllerName.toLower(), controllerName, actimpl);
        fws.write(code, false);
        files << fws.fileName();
    }

    // Generates a project file
    ProjectFileGenerator progen(dir.filePath("controllers.pro"));
    return progen.add(files);
}


QString ControllerGenerator::generateIdString(const QString &id, int type)
{
    return convMethod()->value(type).arg(fieldNameToVariableName(id));
}
