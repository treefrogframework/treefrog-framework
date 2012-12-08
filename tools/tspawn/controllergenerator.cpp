/* Copyright (c) 2010-2012, AOYAMA Kazuharu
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
    "    void renderEntry(const QVariantHash &%3 = QVariantHash());\n"        \
    "    void renderEdit(const QVariantHash &%3 = QVariantHash());\n"         \
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
    "    %2 %3 = %2::get(%4);\n"                               \
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
    "    \n"                                                   \
    "    QVariantHash form = httpRequest().formItems(\"%3\");\n"    \
    "    %2 %3 = %2::create(form);\n"                               \
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
    "void %2Controller::renderEntry(const QVariantHash &%3)\n"      \
    "{\n"                                                           \
    "    texport(%3);\n"                                            \
    "    render(\"entry\");\n"                                      \
    "}\n"                                                           \
    "\n"                                                            \
    "void %2Controller::edit(const QString &pk)\n"                  \
    "{\n"                                                           \
    "    %2 %3 = %2::get(%4);\n"                                    \
    "    if (!%3.isNull()) {\n"                                     \
    "%5"                                                                \
    "        renderEdit(%3.properties());\n"                            \
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
    "    %2 %3 = %2::get(%4%7);\n"                                      \
    "    if (%3.isNull()) {\n"                                          \
    "        error = \"Original data not found. It may have been updated/removed by another transaction.\";\n" \
    "        tflash(error);\n"                                          \
    "        redirect(urla(\"edit\", pk));\n"                           \
    "        return;\n"                                                 \
    "    } \n"                                                          \
    "    \n"                                                            \
    "    QVariantHash form = httpRequest().formItems(\"%3\");\n"        \
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
    "void %2Controller::renderEdit(const QVariantHash &%3)\n"           \
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
    "    \n"                                                            \
    "    %2 %3 = %2::get(%4);\n"                                        \
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


typedef QHash<int, QString> IntHash;
Q_GLOBAL_STATIC_WITH_INITIALIZER(IntHash, convMethod,
{
    x->insert(QVariant::Int,       "pk.toInt()");
    x->insert(QVariant::UInt,      "pk.toUInt()");
    x->insert(QVariant::LongLong,  "pk.toLongLong()");
    x->insert(QVariant::ULongLong, "pk.toULongLong()");
    x->insert(QVariant::Double,    "pk.toDouble()");
    x->insert(QVariant::ByteArray, "pk.toByteArray()");
    x->insert(QVariant::String,    "pk");
    x->insert(QVariant::Date,      "QDate::fromString(pk)");
    x->insert(QVariant::Time,      "QTime::fromString(pk)"); 
    x->insert(QVariant::DateTime,  "QDateTime::fromString(pk)"); 
});


Q_GLOBAL_STATIC_WITH_INITIALIZER(QStringList, ngCtlrName,
{
    *x << "layouts" << "partial" << "direct" << "_src" << "mailer";
})


ControllerGenerator::ControllerGenerator(const QString &controller, const QString &table, const QStringList &actions, const QString &dst)
    : actionList(actions), dstDir(dst)
{
    tableName = (table.contains('_')) ? table.toLower() : variableNameToFieldName(table);
    controllerName = (!controller.isEmpty()) ? controller: fieldNameToEnumName(tableName);
}


bool ControllerGenerator::generate() const
{
    // Reserved word check
    if (ngCtlrName()->contains(tableName.toLower())) {
        qCritical("Reserved word error. Please use another word.  Controller name: %s", qPrintable(tableName));
        return false;
    }

    QStringList files;
    FileWriter fwh(dstDir.filePath(controllerName.toLower() + "controller.h"));
    FileWriter fws(dstDir.filePath(controllerName.toLower() + "controller.cpp"));

    if (actionList.isEmpty()) {
        TableSchema ts(tableName);
        if (ts.primaryKeyIndex() < 0) {
            qWarning("Primary key not found. [table name: %s]", qPrintable(ts.tableName()));
            return false;
        }
                
        // Generates a controller source code
        QString sessInsertStr;
        QString sessGetStr;
        QString revStr;
        QString varName = enumNameToVariableName(controllerName);

        // Generates a controller header file
        QString code = QString(CONTROLLER_HEADER_FILE_TEMPLATE).arg(controllerName.toUpper(), controllerName, varName, controllerName.toLower());
        fwh.write(code, false);
        files << fwh.fileName();

        if (ts.hasLockRevisionField()) {
            sessInsertStr = QString("        session().insert(\"%1_lockRevision\", %1.lockRevision());\n").arg(varName);
            sessGetStr = QString("    int rev = session().value(\"%1_lockRevision\").toInt();\n").arg(varName);
            revStr = QLatin1String(", rev");
        }
        QPair<QString, int> pair = ts.getPrimaryKeyFieldType();
        
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
            actimpl.append("void ").append(controllerName).append("Controller::").append(i.next()).append("()\n{\n    // write codes\n}\n\n");
        }
        code = QString(CONTROLLER_TINY_SOURCE_FILE_TEMPLATE).arg(controllerName.toLower(), controllerName, actimpl);
        fws.write(code, false);
        files << fws.fileName();
    }

    // Generates a project file
    ProjectFileGenerator progen(dstDir.filePath("controllers.pro"));
    return progen.add(files);
}
