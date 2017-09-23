/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include "controllergenerator.h"
#include "projectfilegenerator.h"
#include "filewriter.h"
#include "global.h"
#include "tableschema.h"

#define CONTROLLER_HEADER_FILE_TEMPLATE                                       \
    "#ifndef %1CONTROLLER_H\n"                                                \
    "#define %1CONTROLLER_H\n"                                                \
    "\n"                                                                      \
    "#include \"applicationcontroller.h\"\n"                                  \
    "\n\n"                                                                    \
    "class T_CONTROLLER_EXPORT %2Controller : public ApplicationController\n" \
    "{\n"                                                                     \
    "    Q_OBJECT\n"                                                          \
    "public:\n"                                                               \
    "    %2Controller() : ApplicationController() {}\n"                       \
    "\n"                                                                      \
    "public slots:\n"                                                         \
    "    void index();\n"                                                     \
    "    void show(const QString &%3);\n"                                     \
    "    void create();\n"                                                    \
    "    void save(const QString &%3);\n"                                     \
    "    void remove(const QString &%3);\n"                                   \
    "};\n"                                                                    \
    "\n"                                                                      \
    "#endif // %1CONTROLLER_H\n"


#define CONTROLLER_SOURCE_FILE_TEMPLATE                        \
    "#include \"%1controller.h\"\n"                            \
    "#include \"%1.h\"\n"                                      \
    "\n\n"                                                     \
    "void %2Controller::index()\n"                             \
    "{\n"                                                      \
    "    auto %3List = %2::getAll();\n"                        \
    "    texport(%3List);\n"                                   \
    "    render();\n"                                          \
    "}\n"                                                      \
    "\n"                                                       \
    "void %2Controller::show(const QString &%8)\n"             \
    "{\n"                                                      \
    "    auto %3 = %2::get(%4);\n"                             \
    "    texport(%3);\n"                                       \
    "    render();\n"                                          \
    "}\n"                                                      \
    "\n"                                                       \
    "void %2Controller::create()\n"                            \
    "{\n"                                                      \
    "    switch (httpRequest().method()) {\n"                  \
    "    case Tf::Get:\n"                                      \
    "        render();\n"                                      \
    "        break;\n"                                         \
    "\n"                                                       \
    "    case Tf::Post: {\n"                                   \
    "        auto %3 = httpRequest().formItems(\"%3\");\n"     \
    "        auto model = %2::create(%3);\n"                   \
    "\n"                                                       \
    "        if (!model.isNull()) {\n"                         \
    "            QString notice = \"Created successfully.\";\n" \
    "            tflash(notice);\n"                             \
    "            redirect(urla(\"show\", model.%8()));\n"       \
    "        } else {\n"                                        \
    "            QString error = \"Failed to create.\";\n"      \
    "            texport(error);\n"                             \
    "            texport(%3);\n"                                \
    "            render();\n"                                   \
    "        }\n"                                               \
    "        break; }\n"                                        \
    "\n"                                                        \
    "    default:\n"                                            \
    "        renderErrorResponse(Tf::NotFound);\n"              \
    "        break;\n"                                          \
    "    }\n"                                                   \
    "}\n"                                                       \
    "\n"                                                        \
    "void %2Controller::save(const QString &%8)\n"              \
    "{\n"                                                       \
    "    switch (httpRequest().method()) {\n"                   \
    "    case Tf::Get: {\n"                                     \
    "        auto model = %2::get(%4);\n"                       \
    "        if (!model.isNull()) {\n"                          \
    "%5"                                                        \
    "            auto %3 = model.toVariantMap();\n"             \
    "            texport(%3);\n"                                \
    "            render();\n"                                   \
    "        }\n"                                               \
    "        break; }\n"                                        \
    "\n"                                                        \
    "    case Tf::Post: {\n"                                    \
    "        QString error;\n"                                  \
    "%6"                                                        \
    "        auto model = %2::get(%4%7);\n"                     \
    "        \n"                                                \
    "        if (model.isNull()) {\n"                           \
    "            error = \"Original data not found. It may have been updated/removed by another transaction.\";\n" \
    "            tflash(error);\n"                                      \
    "            redirect(urla(\"save\", %8));\n"                       \
    "            break;\n"                                              \
    "        }\n"                                                       \
    "\n"                                                                \
    "        auto %3 = httpRequest().formItems(\"%3\");\n"              \
    "        model.setProperties(%3);\n"                                \
    "        if (model.save()) {\n"                                     \
    "            QString notice = \"Updated successfully.\";\n"         \
    "            tflash(notice);\n"                                     \
    "            redirect(urla(\"show\", model.%8()));\n"               \
    "        } else {\n"                                                \
    "            error = \"Failed to update.\";\n"                      \
    "            texport(error);\n"                                     \
    "            texport(%3);\n"                                        \
    "            render();\n"                                           \
    "        }\n"                                                       \
    "        break; }\n"                                                \
    "\n"                                                                \
    "    default:\n"                                                    \
    "        renderErrorResponse(Tf::NotFound);\n"                      \
    "        break;\n"                                                  \
    "    }\n"                                                           \
    "}\n"                                                               \
    "\n"                                                                \
    "void %2Controller::remove(const QString &%8)\n"                    \
    "{\n"                                                               \
    "    if (httpRequest().method() != Tf::Post) {\n"                   \
    "        renderErrorResponse(Tf::NotFound);\n"                      \
    "        return;\n"                                                 \
    "    }\n"                                                           \
    "\n"                                                                \
    "    auto %3 = %2::get(%4);\n"                                      \
    "    %3.remove();\n"                                                \
    "    redirect(urla(\"index\"));\n"                                  \
    "}\n"                                                               \
    "\n\n"                                                              \
    "// Don't remove below this line\n"                                 \
    "T_DEFINE_CONTROLLER(%2Controller)\n"


#define CONTROLLER_TINY_HEADER_FILE_TEMPLATE        \
    "#ifndef %1CONTROLLER_H\n"                                                \
    "#define %1CONTROLLER_H\n"                                                \
    "\n"                                                                      \
    "#include \"applicationcontroller.h\"\n"                                  \
    "\n\n"                                                                    \
    "class T_CONTROLLER_EXPORT %2Controller : public ApplicationController\n" \
    "{\n"                                                                     \
    "    Q_OBJECT\n"                                                          \
    "public:\n"                                                               \
    "    %2Controller() : ApplicationController() { }\n"                      \
    "\n"                                                                      \
    "public slots:\n"                                                         \
    "%3"                                                                      \
    "};\n"                                                                    \
    "\n"                                                                      \
    "#endif // %1CONTROLLER_H\n"


#define CONTROLLER_TINY_SOURCE_FILE_TEMPLATE                   \
    "#include \"%1controller.h\"\n"                            \
    "\n\n"                                                     \
    "%3"                                                       \
    "// Don't remove below this line\n"                        \
    "T_DEFINE_CONTROLLER(%2Controller)\n"


class ConvMethod : public QHash<int, QString>
{
public:
    ConvMethod() : QHash<int, QString>()
    {
        insert(QVariant::Int,       "%1.toInt()");
        insert(QVariant::UInt,      "%1.toUInt()");
        insert(QVariant::LongLong,  "%1.toLongLong()");
        insert(QVariant::ULongLong, "%1.toULongLong()");
        insert(QVariant::Double,    "%1.toDouble()");
        insert(QVariant::ByteArray, "%1.toByteArray()");
        insert(QVariant::String,    "%1");
        insert(QVariant::Date,      "QDate::fromString(%1)");
        insert(QVariant::Time,      "QTime::fromString(%1)");
        insert(QVariant::DateTime,  "QDateTime::fromString(%1)");
    }
};
Q_GLOBAL_STATIC(ConvMethod, convMethod)

class NGCtlrName : public QStringList
{
public:
    NGCtlrName() : QStringList()
    {
        append("layouts");
        append("partial");
        append("direct");
        append("_src");
        append("mailer");
    }
};
Q_GLOBAL_STATIC(NGCtlrName, ngCtlrName)


ControllerGenerator::ControllerGenerator(const QString &controller, const QList<QPair<QString, QVariant::Type>> &fields, int pkIdx, int lockRevIdx)
    : controllerName(controller), fieldList(fields), primaryKeyIndex(pkIdx), lockRevIndex(lockRevIdx)
{ }


ControllerGenerator::ControllerGenerator(const QString &controller, const QStringList &actions)
    : controllerName(fieldNameToEnumName(controller)), actionList(actions)
{ }


bool ControllerGenerator::generate(const QString &dstDir) const
{
    // Reserved word check
    if (ngCtlrName()->contains(tableName.toLower())) {
        qCritical("Reserved word error. Please use another word.  Controller name: %s", qPrintable(tableName));
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

        QPair<QString, QVariant::Type> pair;
        if (primaryKeyIndex >= 0)
            pair = fieldList[primaryKeyIndex];

        // Generates a controller source code
        QString sessInsertStr;
        QString sessGetStr;
        QString revStr;
        QString varName = enumNameToVariableName(controllerName);

        // Generates a controller header file
        QString code = QString(CONTROLLER_HEADER_FILE_TEMPLATE).arg(controllerName.toUpper(), controllerName, fieldNameToVariableName(pair.first));
        fwh.write(code, false);
        files << fwh.fileName();

        if (lockRevIndex >= 0) {
            sessInsertStr = QString("            session().insert(\"%1_lockRevision\", model.lockRevision());\n").arg(varName);
            sessGetStr = QString("        int rev = session().value(\"%1_lockRevision\").toInt();\n").arg(varName);
            revStr = QLatin1String(", rev");
        }

        code = QString(CONTROLLER_SOURCE_FILE_TEMPLATE).arg(controllerName.toLower(), controllerName, varName, convMethod()->value(pair.second).arg(fieldNameToVariableName(pair.first)), sessInsertStr, sessGetStr, revStr, fieldNameToVariableName(pair.first));
        fws.write(code, false);
        files << fws.fileName();

    } else {
        // Generates a controller header file
        QString actions;
        for (QStringListIterator i(actionList); i.hasNext(); ) {
            actions.append("    void ").append(i.next()).append("();\n");
        }

        QString code = QString(CONTROLLER_TINY_HEADER_FILE_TEMPLATE).arg(controllerName.toUpper(), controllerName, actions);
        fwh.write(code, false);
        files << fwh.fileName();

        // Generates a controller source code
        QString actimpl;
        for (QStringListIterator i(actionList); i.hasNext(); ) {
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
