/* Copyright (c) 2012-2017, AOYAMA Kazuharu
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
#include "util.h"

#define INDEX_TEMPLATE                                                  \
    "<!DOCTYPE html>\n"                                                 \
    "<%#include \"%1.h\" %>\n"                                          \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta charset=\"UTF-8\">\n"                                      \
    "  <title><%= controller()->name() + \": \" + controller()->activeAction() %></title>\n" \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "\n"                                                                \
    "<h1>Listing %2</h1>\n"                                             \
    "\n"                                                                \
    "<%== linkTo(\"Create a new %2\", urla(\"create\")) %><br />\n"     \
    "<br />\n"                                                          \
    "<table border=\"1\" cellpadding=\"5\" style=\"border: 1px #d0d0d0 solid; border-collapse: collapse;\">\n" \
    "  <tr>\n"                                                          \
    "%3"                                                                \
    "  </tr>\n"                                                         \
    "<% tfetch(QList<%4>, %5List); %>\n"                                \
    "<% for (const auto &i : %5List) { %>\n"                            \
    "  <tr>\n"                                                          \
    "%6"                                                                \
    "    <td>\n"                                                        \
    "      <%== linkTo(\"Show\", urla(\"show\", i.%7())) %>\n"          \
    "      <%== linkTo(\"Edit\", urla(\"save\", i.%7())) %>\n"          \
    "      <%== linkTo(\"Remove\", urla(\"remove\", i.%7()), Tf::Post, \"confirm('Are you sure?')\") %>\n" \
    "    </td>\n"                                                       \
    "  </tr>\n"                                                         \
    "<% } %>\n"                                                         \
    "</table>\n"                                                        \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"

#define SHOW_TEMPLATE                                                   \
    "<!DOCTYPE html>\n"                                                 \
    "<%#include \"%1.h\" %>\n"                                          \
    "<% tfetch(%2, %3); %>\n"                                           \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta charset=\"UTF-8\">\n"                                      \
    "  <title><%= controller()->name() + \": \" + controller()->activeAction() %></title>\n" \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "<p style=\"color: red\"><%=$ error %></p>\n"                       \
    "<p style=\"color: green\"><%=$ notice %></p>\n"                    \
    "\n"                                                                \
    "<h1>Showing %4</h1>\n"                                             \
    "%5"                                                                \
    "\n"                                                                \
    "<%== linkTo(\"Edit\", urla(\"save\", %3.%6())) %> |\n"             \
    "<%== linkTo(\"Back\", urla(\"index\")) %>\n"                       \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"


#define CREATE_TEMPLATE                                                 \
    "<!DOCTYPE html>\n"                                                 \
    "<%#include \"%1.h\" %>\n"                                          \
    "<% tfetch(QVariantMap, %2); %>\n"                                  \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta charset=\"UTF-8\">\n"                                      \
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

#define SAVE_TEMPLATE                                                   \
    "<!DOCTYPE html>\n"                                                 \
    "<%#include \"%1.h\" %>\n"                                          \
    "<% tfetch(QVariantMap, %2); %>\n"                                  \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta charset=\"UTF-8\">\n"                                      \
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
    "    <input type=\"submit\" value=\"Save\" />\n"                    \
    "  </p>\n"                                                          \
    "</form>\n"                                                         \
    "\n"                                                                \
    "<%== linkTo(\"Show\", urla(\"show\", %2[\"%4\"])) %> |\n"          \
    "<%== linkTo(\"Back\", urla(\"index\")) %>\n"                       \
    "</body>\n"                                                         \
    "</html>\n"

static const QStringList excludedColumn = {
    "created_at",
    "updated_at",
    "modified_at",
    "lock_revision",
    "createdAt",
    "updatedAt",
    "modifiedAt",
    "lockRevision",
};


static const QStringList excludedDirName = {
    "layouts",
    "partial",
    "direct",
    "_src",
    "mailer",
};


ErbGenerator::ErbGenerator(const QString &view, const QList<QPair<QString, QVariant::Type>> &fields, int pkIdx, int autoValIdx)
    : viewName(view), fieldList(fields), primaryKeyIndex(pkIdx), autoValueIndex(autoValIdx)
{ }


bool ErbGenerator::generate(const QString &dstDir) const
{
    QDir dir(dstDir + viewName.toLower());

    // Reserved word check
    if (excludedDirName.contains(dir.dirName())) {
        qCritical("Reserved word error. Please use another word.  View name: %s", qPrintable(dir.dirName()));
        return false;
    }
    mkpath(dir);
    copy(dataDirPath + ".trim_mode", dir);

    if (primaryKeyIndex < 0) {
        qWarning("Primary key not found. [view name: %s]", qPrintable(viewName));
        return false;
    }

    FileWriter fw;
    QString output;
    QString caption = enumNameToCaption(viewName);
    QString varName = enumNameToVariableName(viewName);
    const QPair<QString, QVariant::Type> &pkFld = fieldList[primaryKeyIndex];
    QString pkVarName = fieldNameToVariableName(pkFld.first);

    // Generates index.html.erb
    QString th, td, showitems, entryitems, edititems;
    for (int i = 0; i < fieldList.count(); ++i) {
        const QPair<QString, QVariant::Type> &p = fieldList[i];

        QString icap = fieldNameToCaption(p.first);
        QString ivar = fieldNameToVariableName(p.first);

        showitems += "<dt>";
        showitems += icap;
        showitems += "</dt><dd><%= ";
        showitems += varName + "." + ivar;
        showitems += "() %></dd><br />\n";

        if (!excludedColumn.contains(ivar, Qt::CaseInsensitive)) {
            th += "    <th>";
            th += icap;
            th += "</th>\n";

            td += "    <td><%= i.";
            td += ivar;
            td += "() %></td>\n";

            if (i != autoValueIndex) {  // case of not auto-value field
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
    fw.setFilePath(dir.filePath("index.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = QString(SHOW_TEMPLATE).arg(varName.toLower(), viewName, varName, caption, showitems, pkVarName);
    fw.setFilePath(dir.filePath("show.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = QString(CREATE_TEMPLATE).arg(varName.toLower(), varName, caption, entryitems);
    fw.setFilePath(dir.filePath("create.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = QString(SAVE_TEMPLATE).arg(varName.toLower(), varName, caption, pkVarName, edititems);
    fw.setFilePath(dir.filePath("save.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    return true;
}
