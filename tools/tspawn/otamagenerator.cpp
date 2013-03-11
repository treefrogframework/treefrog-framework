/* Copyright (c) 2010-2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "otamagenerator.h"
#include "global.h"
#include "projectfilegenerator.h"
#include "filewriter.h"
#include "tableschema.h"

#define INDEX_HTML_TEMPLATE                                             \
    "<!DOCTYPE HTML>\n"                                                 \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\n" \
    "  <title data-tf=\"@head_title\"></title>\n"                       \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "\n"                                                                \
    "<h1>Listing %1</h1>\n"                                             \
    "\n"                                                                \
    "<a href=\"#\" data-tf=\"@link_to_entry\">New entry</a><br />\n"    \
    "<br />\n"                                                          \
    "<table border=\"1\" cellpadding=\"5\" style=\"border: 1px #d0d0d0 solid; border-collapse: collapse;\">\n" \
    "  <tr>\n"                                                          \
    "%2"                                                                \
    "    <th></th>\n"                                                   \
    "  </tr>\n"                                                         \
    "  <tr data-tf=\"@for\">\n"                                         \
    "%3"                                                                \
    "    <td>\n"                                                        \
    "      <a href=\"#\" data-tf=\"@link_to_show\">Show</a>\n"          \
    "      <a href=\"#\" data-tf=\"@link_to_edit\">Edit</a>\n"          \
    "      <a href=\"#\" data-tf=\"@link_to_remove\">Remove</a>\n"      \
    "    </td>\n"                                                       \
    "  </tr>\n"                                                         \
    "</table>\n"                                                        \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"

#define INDEX_OTM_TEMPLATE                                              \
    "#include \"%1.h\"\n"                                               \
    "\n"                                                                \
    "@head_title ~= controller()->name() + \": \" + controller()->activeAction()\n" \
    "\n"                                                                \
    "@for :\n"                                                          \
    "tfetch(QList<%2>, %3List);\n"                                      \
    "for (QListIterator<%2> it(%3List); it.hasNext(); ) {\n"            \
    "    const %2 &i = it.next();\n"                                    \
    "    %%\n"                                                          \
    "}\n"                                                               \
    "\n"                                                                \
    "%4"                                                                \
    "@link_to_show :== linkTo(\"Show\", urla(\"show\", i.%5()))\n"      \
    "\n"                                                                \
    "@link_to_edit :== linkTo(\"Edit\", urla(\"edit\", i.%5()))\n"      \
    "\n"                                                                \
    "@link_to_remove :== linkTo(\"Remove\", urla(\"remove\", i.%5()), Tf::Post, \"confirm('Are you sure?')\")\n" \
    "\n"                                                                \
    "@link_to_entry :== linkTo(\"New entry\", urla(\"entry\"))\n"

#define SHOW_HTML_TEMPLATE                                              \
    "<!DOCTYPE HTML>\n"                                                 \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\n" \
    "  <title data-tf=\"@head_title\"></title>\n"                       \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "<p style=\"color: red\" data-tf=\"@error_msg\"></p>\n"             \
    "<p style=\"color: green\" data-tf=\"@notice_msg\"></p>\n"          \
    "\n"                                                                \
    "<h1>Showing %1</h1>\n"                                             \
    "%2"                                                                \
    "\n"                                                                \
    "<a href=\"#\" data-tf=\"@link_to_edit\">Edit</a> |\n"              \
    "<a href=\"#\" data-tf=\"@link_to_index\">Back</a>\n"               \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"

#define SHOW_OTM_TEMPLATE                                               \
    "#include \"%1.h\"\n"                                               \
    "\n"                                                                \
    "#init\n"                                                           \
    " tfetch(%2, %3);\n"                                                \
    "\n"                                                                \
    "@head_title ~= controller()->name() + \": \" + controller()->activeAction()\n" \
    "\n"                                                                \
    "@error_msg ~=$ error\n"                                            \
    "\n"                                                                \
    "@notice_msg ~=$ notice\n"                                          \
    "\n"                                                                \
    "%4"                                                                \
    "@link_to_edit :== linkTo(\"Edit\", urla(\"edit\", %3.%5()))\n"     \
    "\n"                                                                \
    "@link_to_index :== linkTo(\"Back\", urla(\"index\"))\n"

#define ENTRY_HTML_TEMPLATE                                             \
    "<!DOCTYPE HTML>\n"                                                 \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\n" \
    "  <title data-tf=\"@head_title\"></title>\n"                       \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "<p style=\"color: red\" data-tf=\"@error_msg\"></p>\n"             \
    "<p style=\"color: green\" data-tf=\"@notice_msg\"></p>\n"          \
    "\n"                                                                \
    "<h1>New %1</h1>\n"                                                 \
    "\n"                                                                \
    "<form method=\"post\" data-tf=\"@entry_form\">\n"                  \
    "%2"                                                                \
    "  <p>\n"                                                           \
    "    <input type=\"submit\" value=\"Create\" />\n"                  \
    "  </p>\n"                                                          \
    "</form>\n"                                                         \
    "\n"                                                                \
    "<a href=\"#\" data-tf=\"@link_to_index\">Back</a>\n"               \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"

#define ENTRY_OTM_TEMPLATE                                              \
    "#include \"%1.h\"\n"                                               \
    "\n"                                                                \
    "#init\n"                                                           \
    " tfetch(QVariantMap, %2);\n"                                       \
    "\n"                                                                \
    "@head_title ~= controller()->name() + \": \" + controller()->activeAction()\n" \
    "\n"                                                                \
    "@error_msg ~=$ error\n"                                            \
    "\n"                                                                \
    "@notice_msg ~=$ notice\n"                                          \
    "\n"                                                                \
    "@entry_form |== formTag(urla(\"create\"))\n"                       \
    "\n"                                                                \
    "%3"                                                                \
    "@link_to_index |== linkTo(\"Back\", urla(\"index\"))\n"

#define EDIT_HTML_TEMPLATE                                              \
    "<!DOCTYPE HTML>\n"                                                 \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\n" \
    "  <title data-tf=\"@head_title\"></title>\n"                       \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "<p style=\"color: red\" data-tf=\"@error_msg\"></p>\n"             \
    "<p style=\"color: green\" data-tf=\"@notice_msg\"></p>\n"          \
    "\n"                                                                \
    "<h1>Editing %1</h1>\n"                                             \
    "\n"                                                                \
    "<form method=\"post\" data-tf=\"@edit_form\">\n"                   \
    "%2"                                                                \
    "  <p>\n"                                                           \
    "    <input type=\"submit\" value=\"Update\" />\n"                  \
    "  </p>\n"                                                          \
    "</form>\n"                                                         \
    "\n"                                                                \
    "<a href=\"#\" data-tf=\"@link_to_show\">Show</a> |\n"              \
    "<a href=\"#\" data-tf=\"@link_to_index\">Back</a>\n"               \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"

#define EDIT_OTM_TEMPLATE                                               \
    "#include \"%1.h\"\n"                                               \
    "\n"                                                                \
    "#init\n"                                                           \
    " tfetch(QVariantMap, %2);\n"                                       \
    "\n"                                                                \
    "@head_title ~= controller()->name() + \": \" + controller()->activeAction()\n" \
    "\n"                                                                \
    "@error_msg ~=$ error\n"                                            \
    "\n"                                                                \
    "@notice_msg ~=$ notice\n"                                          \
    "\n"                                                                \
    "@edit_form |== formTag(urla(\"save\", %2[\"%3\"]))\n"              \
    "\n"                                                                \
    "%5"                                                                \
    "@link_to_show |== linkTo(\"Show\", urla(\"show\", %2[\"%3\"]))\n"  \
    "\n"                                                                \
    "@link_to_index |== linkTo(\"Back\", urla(\"index\"))\n"

Q_GLOBAL_STATIC_WITH_INITIALIZER(QStringList, excludedColumn,
{
    *x << "created_at" << "updated_at" << "modified_at" << "lock_revision";
})


Q_GLOBAL_STATIC_WITH_INITIALIZER(QStringList, excludedDirName,
{
    *x << "layouts" << "partial" << "direct" << "_src" << "mailer";
})


OtamaGenerator::OtamaGenerator(const QString &view, const QString &table, const QString &dst)
{
    tableName = (table.contains('_')) ? table.toLower() : variableNameToFieldName(table);
    viewName = (!view.isEmpty()) ? view : fieldNameToEnumName(tableName);
    dstDir.setPath(dst + viewName.toLower());
}


bool OtamaGenerator::generate() const
{
    // Reserved word check
    if (excludedDirName()->contains(dstDir.dirName())) {
        qCritical("Reserved word error. Please use another word.  View name: %s", qPrintable(dstDir.dirName()));
        return false;
    }

    mkpath(dstDir);

    if (!TableSchema(tableName).exists()) {
        qCritical("table not found, %s", qPrintable(tableName));
        return false;
    }

    // Generates view files
    generateViews();
    return true;
}


QStringList OtamaGenerator::generateViews() const
{
    QStringList files;
    TableSchema ts(tableName);
    if (ts.primaryKeyIndex() < 0) {
        qWarning("Primary key not found. [table name: %s]", qPrintable(ts.tableName()));
        return files;
    }
    
    FileWriter fw;
    QString output;
    QString caption = enumNameToCaption(viewName);
    QString varName = enumNameToVariableName(viewName);
    QPair<QString, QString> pkFld = ts.getPrimaryKeyField();
    int autoidx = ts.autoValueIndex();
    
    // Generates index.html
    QString th ,td, indexOtm, showColumn, showOtm, entryColumn, editColumn, entryOtm, editOtm;
    QList<QPair<QString, QString> > fields = ts.getFieldList();
    for (int i = 0; i < fields.count(); ++i) {
        const QPair<QString, QString> &p = fields[i];
        QString cap = fieldNameToCaption(p.first);
        QString var = fieldNameToVariableName(p.first);
        QString mrk = p.first.toLower();
        QString readonly;

        if (!excludedColumn()->contains(p.first)) {
            th += "    <th>";
            th += cap;
            th += "</th>\n";

            td += "    <td data-tf=\"@";
            td += mrk;
            td += "\"></td>\n";

            indexOtm += QString("@%1 ~= i.%2()\n\n").arg(mrk, var);

            if (i != autoidx) {  // case of not auto-value field
                entryColumn += QString("  <p>\n    <label>%1<br /><input data-tf=\"@%2\" /></label>\n  </p>\n").arg(cap, mrk);
                entryOtm += QString("@%1 |== inputTextTag(\"%2[%3]\", %2[\"%3\"].toString())\n\n").arg(mrk, varName, var);
            }

            editColumn += QString("  <p>\n    <label>%1<br /><input data-tf=\"@%2\" /></label>\n  </p>\n").arg(cap, mrk);
            if  (p.first == pkFld.first) {
                readonly = QLatin1String(", a(\"readonly\", \"readonly\")");
            }
            editOtm += QString("@%1 |== inputTextTag(\"%2[%3]\", %2[\"%3\"].toString()%4);\n\n").arg(mrk, varName, var, readonly);
        }
        showColumn += QString("<dt>%1</dt><dd data-tf=\"@%2\">(%3)</dd><br />\n").arg(cap, mrk, var);
        showOtm += QString("@%1 ~= %2.%3()\n\n").arg(mrk, varName, var);
    }
    
    output = QString(INDEX_HTML_TEMPLATE).arg(caption, th, td);
    fw.setFilePath(dstDir.filePath("index.html"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates index.otm
    QString pkVarName = fieldNameToVariableName(pkFld.first);
    output = QString(INDEX_OTM_TEMPLATE).arg(varName.toLower(), viewName, varName, indexOtm, pkVarName);
    fw.setFilePath(dstDir.filePath("index.otm"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates show.html
    output = QString(SHOW_HTML_TEMPLATE).arg(caption, showColumn);
    fw.setFilePath(dstDir.filePath("show.html"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates show.otm
    output = QString(SHOW_OTM_TEMPLATE).arg(varName.toLower(), viewName, varName, showOtm, pkVarName);
    fw.setFilePath(dstDir.filePath("show.otm"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates entry.html
    output = QString(ENTRY_HTML_TEMPLATE).arg(caption, entryColumn);
    fw.setFilePath(dstDir.filePath("entry.html"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates entry.otm
    output = QString(ENTRY_OTM_TEMPLATE).arg(varName.toLower(), varName, entryOtm);
    fw.setFilePath(dstDir.filePath("entry.otm"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates edit.html
    output = QString(EDIT_HTML_TEMPLATE).arg(caption, editColumn);
    fw.setFilePath(dstDir.filePath("edit.html"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates edit.otm
    output = QString(EDIT_OTM_TEMPLATE).arg(varName.toLower(), varName, pkVarName, editOtm);
    fw.setFilePath(dstDir.filePath("edit.otm"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    return files;
}
