/* Copyright (c) 2010-2015, AOYAMA Kazuharu
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
    "    %2Controller() { }\n"                                                \
    "    %2Controller(const %2Controller &other);\n"                          \
    "\n"                                                                      \
    "public slots:\n"                                                         \
    "    void index();\n"                                                     \
    "    void show(const QString &pk);\n"                                     \
    "    void entry();\n"                                                     \
    "    void create();\n"                                                    \
    "    void edit(const QString &pk);\n"                                     \
    "    void save(const QString &pk);\n"                                     \
    "    void remove(const QString &pk);\n"                                   \
    "\n"                                                                      \
    "private:\n"                                                              \
    "    void renderEntry(const QVariantMap &%3 = QVariantMap());\n"          \
    "    void renderEdit(const QVariantMap &%3 = QVariantMap());\n"           \
    "};\n"                                                                    \
    "\n"                                                                      \
    "T_DECLARE_CONTROLLER(%2Controller, %4controller)\n"                      \
    "\n"                                                                      \
    "#endif // %1CONTROLLER_H\n"


#define CONTROLLER_SOURCE_FILE_TEMPLATE                        \
    "#include \"%1controller.h\"\n"                            \
    "#include \"%1.h\"\n"                                      \
    "\n\n"                                                     \
    "%2Controller::%2Controller(const %2Controller &)\n"       \
    "    : ApplicationController()\n"                          \
    "{ }\n"                                                    \
    "\n"                                                       \
    "void %2Controller::index()\n"                             \
    "{\n"                                                      \
    "    QList<%2> %3List = %2::getAll();\n"                   \
    "    texport(%3List);\n"                                   \
    "    render();\n"                                          \
    "}\n"                                                      \
    "\n"                                                       \
    "void %2Controller::show(const QString &pk)\n"             \
    "{\n"                                                      \
    "    auto %3 = %2::get(%4);\n"                             \
    "    texport(%3);\n"                                       \
    "    render();\n"                                          \
    "}\n"                                                      \
    "\n"                                                       \
    "void %2Controller::entry()\n"                             \
    "{\n"                                                      \
    "    renderEntry();\n"                                     \
    "}\n"                                                      \
    "\n"                                                       \
    "void %2Controller::create()\n"                            \
    "{\n"                                                      \
    "    if (httpRequest().method() != Tf::Post) {\n"          \
    "        return;\n"                                        \
    "    }\n"                                                  \
    "\n"                                                       \
    "    auto form = httpRequest().formItems(\"%3\");\n"            \
    "    auto %3 = %2::create(form);\n"                             \
    "    if (!%3.isNull()) {\n"                                     \
    "        QString notice = \"Created successfully.\";\n"         \
    "        tflash(notice);\n"                                     \
    "        redirect(urla(\"show\", %3.%8()));\n"                  \
    "    } else {\n"                                                \
    "        QString error = \"Failed to create.\";\n"              \
    "        texport(error);\n"                                     \
    "        renderEntry(form);\n"                                  \
    "    }\n"                                                       \
    "}\n"                                                           \
    "\n"                                                            \
    "void %2Controller::renderEntry(const QVariantMap &%3)\n"       \
    "{\n"                                                           \
    "    texport(%3);\n"                                            \
    "    render(\"entry\");\n"                                      \
    "}\n"                                                           \
    "\n"                                                            \
    "void %2Controller::edit(const QString &pk)\n"                  \
    "{\n"                                                           \
    "    auto %3 = %2::get(%4);\n"                                  \
    "    if (!%3.isNull()) {\n"                                     \
    "%5"                                                                \
    "        renderEdit(%3.toVariantMap());\n"                          \
    "    } else {\n"                                                    \
    "        redirect(urla(\"entry\"));\n"                              \
    "    }\n"                                                           \
    "}\n"                                                               \
    "\n"                                                                \
    "void %2Controller::save(const QString &pk)\n"                      \
    "{\n"                                                               \
    "    if (httpRequest().method() != Tf::Post) {\n"                   \
    "        return;\n"                                                 \
    "    }\n"                                                           \
    "\n"                                                                \
    "    QString error;\n"                                              \
    "%6"                                                                \
    "    auto %3 = %2::get(%4%7);\n"                                    \
    "    if (%3.isNull()) {\n"                                          \
    "        error = \"Original data not found. It may have been updated/removed by another transaction.\";\n" \
    "        tflash(error);\n"                                          \
    "        redirect(urla(\"edit\", pk));\n"                           \
    "        return;\n"                                                 \
    "    }\n"                                                           \
    "\n"                                                                \
    "    auto form = httpRequest().formItems(\"%3\");\n"                \
    "    %3.setProperties(form);\n"                                     \
    "    if (%3.save()) {\n"                                            \
    "        QString notice = \"Updated successfully.\";\n"             \
    "        tflash(notice);\n"                                         \
    "        redirect(urla(\"show\", pk));\n"                           \
    "    } else {\n"                                                    \
    "        error = \"Failed to update.\";\n"                          \
    "        texport(error);\n"                                         \
    "        renderEdit(form);\n"                                       \
    "    }\n"                                                           \
    "}\n"                                                               \
    "\n"                                                                \
    "void %2Controller::renderEdit(const QVariantMap &%3)\n"            \
    "{\n"                                                               \
    "    texport(%3);\n"                                                \
    "    render(\"edit\");\n"                                           \
    "}\n"                                                               \
    "\n"                                                                \
    "void %2Controller::remove(const QString &pk)\n"                    \
    "{\n"                                                               \
    "    if (httpRequest().method() != Tf::Post) {\n"                   \
    "        return;\n"                                                 \
    "    }\n"                                                           \
    "\n"                                                                \
    "    auto %3 = %2::get(%4);\n"                                      \
    "    %3.remove();\n"                                                \
    "    redirect(urla(\"index\"));\n"                                  \
    "}\n"                                                               \
    "\n\n"                                                              \
    "// Don't remove below this line\n"                                 \
    "T_REGISTER_CONTROLLER(%1controller)\n"


#define CONTROLLER_TINY_HEADER_FILE_TEMPLATE                                  \
    "#ifndef %1CONTROLLER_H\n"                                                \
    "#define %1CONTROLLER_H\n"                                                \
    "\n"                                                                      \
    "#include \"applicationcontroller.h\"\n"                                  \
    "\n\n"                                                                    \
    "class T_CONTROLLER_EXPORT %2Controller : public ApplicationController\n" \
    "{\n"                                                                     \
    "    Q_OBJECT\n"                                                          \
    "public:\n"                                                               \
    "    %2Controller() { }\n"                                                \
    "    %2Controller(const %2Controller &other);\n"                          \
    "\n"                                                                      \
    "public slots:\n"                                                         \
    "%3"                                                                      \
    "};\n"                                                                    \
    "\n"                                                                      \
    "T_DECLARE_CONTROLLER(%2Controller, %4controller)\n"                      \
    "\n"                                                                      \
    "#endif // %1CONTROLLER_H\n"


#define CONTROLLER_TINY_SOURCE_FILE_TEMPLATE                   \
    "#include \"%1controller.h\"\n"                            \
    "\n\n"                                                     \
    "%2Controller::%2Controller(const %2Controller &)\n"       \
    "    : ApplicationController()\n"                          \
    "{ }\n"                                                    \
    "\n"                                                       \
    "%3\n"                                                     \
    "// Don't remove below this line\n"                        \
    "T_REGISTER_CONTROLLER(%1controller)\n"


class ConvMethod : public QHash<int, QString>
{
public:
    ConvMethod() : QHash<int, QString>()
    {
        insert(QVariant::Int,       "pk.toInt()");
        insert(QVariant::UInt,      "pk.toUInt()");
        insert(QVariant::LongLong,  "pk.toLongLong()");
        insert(QVariant::ULongLong, "pk.toULongLong()");
        insert(QVariant::Double,    "pk.toDouble()");
        insert(QVariant::ByteArray, "pk.toByteArray()");
        insert(QVariant::String,    "pk");
        insert(QVariant::Date,      "QDate::fromString(pk)");
        insert(QVariant::Time,      "QTime::fromString(pk)");
        insert(QVariant::DateTime,  "QDateTime::fromString(pk)");
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
        QString code = QString(CONTROLLER_HEADER_FILE_TEMPLATE).arg(controllerName.toUpper(), controllerName, varName, controllerName.toLower());
        fwh.write(code, false);
        files << fwh.fileName();

        if (lockRevIndex >= 0) {
            sessInsertStr = QString("        session().insert(\"%1_lockRevision\", %1.lockRevision());\n").arg(varName);
            sessGetStr = QString("    int rev = session().value(\"%1_lockRevision\").toInt();\n").arg(varName);
            revStr = QLatin1String(", rev");
        }

        code = QString(CONTROLLER_SOURCE_FILE_TEMPLATE).arg(controllerName.toLower(), controllerName, varName, convMethod()->value(pair.second), sessInsertStr, sessGetStr, revStr, fieldNameToVariableName(pair.first));
        fws.write(code, false);
        files << fws.fileName();

    } else {
        // Generates a controller header file
        QString actions;
        for (QStringListIterator i(actionList); i.hasNext(); ) {
            actions.append("    void ").append(i.next()).append("();\n");
        }

        QString code = QString(CONTROLLER_TINY_HEADER_FILE_TEMPLATE).arg(controllerName.toUpper(), controllerName, actions, controllerName.toLower());
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
