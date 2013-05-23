/* Copyright (c) 2012, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QList>
#include <QPair>
#include "erbgenerator.h"
#include "global.h"
#include "filewriter.h"
#include "tableschema.h"
#include "util.h"

#define INDEX_TEMPLATE                                                  \
    "<!DOCTYPE HTML>\n"                                                 \
    "<%#include \"%1.h\" %>\n"                                          \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\n" \
    "  <title><%= controller()->name() + \": \" + controller()->activeAction() %></title>\n" \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "\n"                                                                \
    "<h1>Listing %2</h1>\n"                                             \
    "\n"                                                                \
    "<%== linkTo(\"New entry\", urla(\"entry\")) %><br />\n"            \
    "<br />\n"                                                          \
    "<table border=\"1\" cellpadding=\"5\" style=\"border: 1px #d0d0d0 solid; border-collapse: collapse;\">\n" \
    "  <tr>\n"                                                          \
    "%3"                                                                \
    "  </tr>\n"                                                         \
    "<% tfetch(QList<%4>, %5List); %>\n"                                \
    "<% for (QListIterator<%4> it(%5List); it.hasNext(); ) {\n"         \
    "     const %4 &i = it.next(); %>\n"                                \
    "  <tr>\n"                                                          \
    "%6"                                                                \
    "    <td>\n"                                                        \
    "      <%== linkTo(\"Show\", urla(\"show\", i.%7())) %>\n"          \
    "      <%== linkTo(\"Edit\", urla(\"edit\", i.%7())) %>\n"          \
    "      <%== linkTo(\"Remove\", urla(\"remove\", i.%7()), Tf::Post, \"confirm('Are you sure?')\") %>\n" \
    "    </td>\n"                                                       \
    "  </tr>\n"                                                         \
    "<% } %>\n"                                                         \
    "</table>\n"                                                        \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"

#define SHOW_TEMPLATE                                                   \
    "<!DOCTYPE HTML>\n"                                                 \
    "<%#include \"%1.h\" %>\n"                                          \
    "<% tfetch(%2, %3); %>\n"                                           \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\n" \
    "  <title><%= controller()->name() + \": \" + controller()->activeAction() %></title>\n" \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "<p style=\"color: red\"><%=$ error %></p>\n"                       \
    "<p style=\"color: green\"><%=$ notice %></p>\n"                    \
    "\n"                                                                \
    "<h1>Showing %4</h1>\n"                                             \
    "%5"                                                                \
    "\n"                                                                \
    "<%== linkTo(\"Edit\", urla(\"edit\", %3.%6())) %> |\n"             \
    "<%== linkTo(\"Back\", urla(\"index\")) %>\n"                       \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"


#define ENTRY_TEMPLATE                                                  \
    "<!DOCTYPE HTML>\n"                                                 \
    "<%#include \"%1.h\" %>\n"                                          \
    "<% tfetch(QVariantMap, %2); %>\n"                                  \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\n" \
    "  <title><%= controller()->name() + \": \" + controller()->activeAction() %></title>\n" \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "<p style=\"color: red\"><%=$ error %></p>\n"                       \
    "<p style=\"color: green\"><%=$ notice %></p>\n"                    \
    "\n"                                                                \
    "<h1>New %3</h1>\n"                                                 \
    "\n"                                                                \
    "<%== formTag(urla(\"create\"), Tf::Post) %>\n"                     \
    "%4"                                                                \
    "  <p>\n"                                                           \
    "    <input type=\"submit\" value=\"Create\" />\n"                  \
    "  </p>\n"                                                          \
    "</form>\n"                                                         \
    "\n"                                                                \
    "<%== linkTo(\"Back\", urla(\"index\")) %>\n"                       \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"

#define EDIT_TEMPLATE                                                   \
    "<!DOCTYPE HTML>\n"                                                 \
    "<%#include \"%1.h\" %>\n"                                          \
    "<% tfetch(QVariantMap, %2); %>\n"                                  \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\" />\n" \
    "  <title><%= controller()->name() + \": \" + controller()->activeAction() %></title>\n" \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "<p style=\"color: red\"><%=$ error %></p>\n"                       \
    "<p style=\"color: green\"><%=$ notice %></p>\n"                    \
    "\n"                                                                \
    "<h1>Editing %3</h1>\n"                                             \
    "\n"                                                                \
    "<%== formTag(urla(\"save\", %2[\"%4\"]), Tf::Post) %>\n"           \
    "%6"                                                                \
    "  <p>\n"                                                           \
    "    <input type=\"submit\" value=\"Update\" />\n"                  \
    "  </p>\n"                                                          \
    "</form>\n"                                                         \
    "\n"                                                                \
    "<%== linkTo(\"Show\", urla(\"show\", %2[\"%4\"])) %> |\n"          \
    "<%== linkTo(\"Back\", urla(\"index\")) %>\n"                       \
    "</body>\n"                                                         \
    "</html>\n"

Q_GLOBAL_STATIC_WITH_INITIALIZER(QStringList, excludedColumn,
{
    *x << "created_at" << "updated_at" << "modified_at" << "lock_revision";
})


Q_GLOBAL_STATIC_WITH_INITIALIZER(QStringList, excludedDirName,
{
    *x << "layouts" << "partial" << "direct" << "_src" << "mailer";
})


ErbGenerator::ErbGenerator(const QString &view, const QString &table, const QString &dst)
    : viewName(), tableName(table)
{
    viewName = (!view.isEmpty()) ? view : fieldNameToEnumName(table);
    dstDir.setPath(dst + viewName.toLower());
}


bool ErbGenerator::generate() const
{
    // Reserved word check
    if (excludedDirName()->contains(dstDir.dirName())) {
        qCritical("Reserved word error. Please use another word.  View name: %s", qPrintable(dstDir.dirName()));
        return false;
    }
    mkpath(dstDir);

    FileWriter fw;
    QString output;
    TableSchema ts(tableName);
    if (ts.primaryKeyIndex() < 0) {
        qWarning("Primary key not found. [table name: %s]", qPrintable(ts.tableName()));
        qWarning("Unable to create template files");
        return false;
    }

    QString caption = enumNameToCaption(viewName);
    QString varName = enumNameToVariableName(viewName);
    QPair<QString, QString> pkFld = ts.getPrimaryKeyField();
    QString pkVarName = fieldNameToVariableName(pkFld.first);
    int autoidx = ts.autoValueIndex();

    // Generates index.html.erb
    QString th, td, showitems, entryitems, edititems;
    QList<QPair<QString, QString> > fields = ts.getFieldList();
    for (int i = 0; i < fields.count(); ++i) {
        const QPair<QString, QString> &p = fields[i];

        QString icap = fieldNameToCaption(p.first);
        QString ivar = fieldNameToVariableName(p.first);

        showitems += "<dt>";
        showitems += icap;
        showitems += "</dt><dd><%= ";
        showitems += varName + "." + ivar;
        showitems += "() %></dd><br />\n";

        if (!excludedColumn()->contains(p.first, Qt::CaseInsensitive)) {
            th += "    <th>";
            th += icap;
            th += "</th>\n";

            td += "    <td><%= i.";
            td += ivar;
            td += "() %></td>\n";

            if (i != autoidx) {  // case of not auto-value field
                entryitems += "  <p>\n    <label>";
                entryitems += icap;
                entryitems += "<br /><input name=\"";
                entryitems += varName + '[' + ivar + ']';
                entryitems += "\" value=\"<%= ";
                entryitems += varName + "[\"" + ivar + "\"]";
                entryitems += " %>\" /></label>\n  </p>\n";
            }
            edititems += "  <p>\n    <label>";
            edititems += icap;
            edititems += "<br /><input type=\"text\" name=\"";
            edititems += varName + '[' + ivar + ']';
            edititems += "\" value=\"<%= ";
            edititems += varName + "[\"" + ivar + "\"]";
            edititems += " %>\"";
            if (p.first == pkFld.first) {
                edititems += " readonly=\"readonly\"";
            }
            edititems += " /></label>\n  </p>\n";
        }
    }

    output = QString(INDEX_TEMPLATE).arg(varName.toLower(), caption, th, viewName, varName,td, pkVarName);
    fw.setFilePath(dstDir.filePath("index.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = QString(SHOW_TEMPLATE).arg(varName.toLower(), viewName, varName, caption, showitems, pkVarName);
    fw.setFilePath(dstDir.filePath("show.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = QString(ENTRY_TEMPLATE).arg(varName.toLower(), varName, caption, entryitems);
    fw.setFilePath(dstDir.filePath("entry.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = QString(EDIT_TEMPLATE).arg(varName.toLower(), varName, caption, pkVarName, edititems);
    fw.setFilePath(dstDir.filePath("edit.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    return true;
}
