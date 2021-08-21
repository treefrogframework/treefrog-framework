/* Copyright (c) 2012-2021, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "erbgenerator.h"
#include "filewriter.h"
#include "util.h"
#include <QList>
#include <QPair>

constexpr auto INDEX_TEMPLATE = "<!DOCTYPE html>\n"
                                "<%#include \"objects/%varname%.h\" %>\n"
                                "<html>\n"
                                "<head>\n"
                                "  <meta charset=\"UTF-8\">\n"
                                "  <title><%= controller()->name() + \": \" + controller()->activeAction() %></title>\n"
                                "</head>\n"
                                "<body>\n"
                                "\n"
                                "<h1>Listing %caption%</h1>\n"
                                "\n"
                                "<%== linkTo(\"Create a new %caption%\", urla(\"create\")) %><br>\n"
                                "<br>\n"
                                "<table border=\"1\" cellpadding=\"5\" style=\"border: 1px #d0d0d0 solid; border-collapse: collapse;\">\n"
                                "  <tr>\n"
                                "%th%"
                                "  </tr>\n"
                                "<% tfetch(QList<%viewName%>, %varName%List); %>\n"
                                "<% for (const auto &i : %varName%List) { %>\n"
                                "  <tr>\n"
                                "%td%"
                                "    <td>\n"
                                "      <%== linkTo(\"Show\", urla(\"show\", i.%pkVarName%())) %>\n"
                                "      <%== linkTo(\"Edit\", urla(\"save\", i.%pkVarName%())) %>\n"
                                "      <%== linkTo(\"Remove\", urla(\"remove\", i.%pkVarName%()), Tf::Post, \"confirm('Are you sure?')\") %>\n"
                                "    </td>\n"
                                "  </tr>\n"
                                "<% } %>\n"
                                "</table>\n"
                                "\n"
                                "</body>\n"
                                "</html>\n";

constexpr auto SHOW_TEMPLATE = "<!DOCTYPE html>\n"
                               "<%#include \"objects/%varname%.h\" %>\n"
                               "<% tfetch(%viewName%, %varName%); %>\n"
                               "<html>\n"
                               "<head>\n"
                               "  <meta charset=\"UTF-8\">\n"
                               "  <title><%= controller()->name() + \": \" + controller()->activeAction() %></title>\n"
                               "</head>\n"
                               "<body>\n"
                               "<p style=\"color: red\"><%=$ error %></p>\n"
                               "<p style=\"color: green\"><%=$ notice %></p>\n"
                               "\n"
                               "<h1>Showing %caption%</h1>\n"
                               "%showitems%"
                               "\n"
                               "<%== linkTo(\"Edit\", urla(\"save\", %varName%.%pkVarName%())) %> |\n"
                               "<%== linkTo(\"Back\", urla(\"index\")) %>\n"
                               "\n"
                               "</body>\n"
                               "</html>\n";


constexpr auto CREATE_TEMPLATE = "<!DOCTYPE html>\n"
                                 "<%#include \"objects/%varname%.h\" %>\n"
                                 "<% tfetch(QVariantMap, %varName%); %>\n"
                                 "<html>\n"
                                 "<head>\n"
                                 "  <meta charset=\"UTF-8\">\n"
                                 "  <title><%= controller()->name() + \": \" + controller()->activeAction() %></title>\n"
                                 "</head>\n"
                                 "<body>\n"
                                 "<p style=\"color: red\"><%=$ error %></p>\n"
                                 "<p style=\"color: green\"><%=$ notice %></p>\n"
                                 "\n"
                                 "<h1>New %caption%</h1>\n"
                                 "\n"
                                 "<%== formTag(urla(\"create\"), Tf::Post) %>\n"
                                 "%entryitems%"
                                 "  <p>\n"
                                 "    <input type=\"submit\" value=\"Create\" />\n"
                                 "  </p>\n"
                                 "</form>\n"
                                 "\n"
                                 "<%== linkTo(\"Back\", urla(\"index\")) %>\n"
                                 "\n"
                                 "</body>\n"
                                 "</html>\n";

constexpr auto SAVE_TEMPLATE = "<!DOCTYPE html>\n"
                               "<%#include \"objects/%varname%.h\" %>\n"
                               "<% tfetch(QVariantMap, %varName%); %>\n"
                               "<html>\n"
                               "<head>\n"
                               "  <meta charset=\"UTF-8\">\n"
                               "  <title><%= controller()->name() + \": \" + controller()->activeAction() %></title>\n"
                               "</head>\n"
                               "<body>\n"
                               "<p style=\"color: red\"><%=$ error %></p>\n"
                               "<p style=\"color: green\"><%=$ notice %></p>\n"
                               "\n"
                               "<h1>Editing %caption%</h1>\n"
                               "\n"
                               "<%== formTag(urla(\"save\", %varName%[\"%pkVarName%\"]), Tf::Post) %>\n"
                               "%edititems%"
                               "  <p>\n"
                               "    <input type=\"submit\" value=\"Save\" />\n"
                               "  </p>\n"
                               "</form>\n"
                               "\n"
                               "<%== linkTo(\"Show\", urla(\"show\", %varName%[\"%pkVarName%\"])) %> |\n"
                               "<%== linkTo(\"Back\", urla(\"index\")) %>\n"
                               "</body>\n"
                               "</html>\n";


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


ErbGenerator::ErbGenerator(const QString &view, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int autoValIdx) :
    _viewName(view), _fieldList(fields), _primaryKeyIndex(pkIdx), _autoValueIndex(autoValIdx)
{
}


PlaceholderList ErbGenerator::replaceList() const
{
    QString caption = enumNameToCaption(_viewName);
    QString varName = enumNameToVariableName(_viewName);
    const QPair<QString, QMetaType::Type> &pkFld = _fieldList[_primaryKeyIndex];
    QString pkVarName = fieldNameToVariableName(pkFld.first);

    // Generates index.html.erb
    QString th, td, showitems, entryitems, edititems;
    for (int i = 0; i < _fieldList.count(); ++i) {
        const QPair<QString, QMetaType::Type> &p = _fieldList[i];

        QString icap = fieldNameToCaption(p.first);
        QString ivar = fieldNameToVariableName(p.first);

        showitems += "<dt>";
        showitems += icap;
        showitems += "</dt><dd><%= ";
        showitems += varName + "." + ivar;
        showitems += "() %></dd><br>\n";

        if (!excludedColumn.contains(ivar, Qt::CaseInsensitive)) {
            th += "    <th>";
            th += icap;
            th += "</th>\n";

            td += "    <td><%= i.";
            td += ivar;
            td += "() %></td>\n";

            if (i != _autoValueIndex) {  // case of not auto-value field
                entryitems += "  <p>\n    <label>";
                entryitems += icap;
                entryitems += "<br><input name=\"";
                entryitems += varName + '[' + ivar + ']';
                entryitems += "\" value=\"<%= ";
                entryitems += varName + "[\"" + ivar + "\"]";
                entryitems += " %>\" /></label>\n  </p>\n";
            }
            edititems += "  <p>\n    <label>";
            edititems += icap;
            edititems += "<br><input type=\"text\" name=\"";
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

    PlaceholderList list = {
        {"varname", varName.toLower()},
        {"caption", caption},
        {"th", th},
        {"viewName", _viewName},
        {"varName", varName},
        {"td", td},
        {"pkVarName", pkVarName},
        {"showitems", showitems},
        {"entryitems", entryitems},
        {"edititems", edititems},
    };
    return list;
}


bool ErbGenerator::generate(const QString &dstDir) const
{
    QDir dir(dstDir + _viewName.toLower());

    // Reserved word check
    if (excludedDirName.contains(dir.dirName())) {
        qCritical("Reserved word error. Please use another word.  View name: %s", qUtf8Printable(dir.dirName()));
        return false;
    }
    mkpath(dir);
    copy(dataDirPath + ".trim_mode", dir);

    if (_primaryKeyIndex < 0) {
        qWarning("Primary key not found. [view name: %s]", qUtf8Printable(_viewName));
        return false;
    }

    FileWriter fw;
    QString output;

    output = replaceholder(indexTemplate(), replaceList());
    fw.setFilePath(dir.filePath("index.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = replaceholder(showTemplate(), replaceList());
    fw.setFilePath(dir.filePath("show.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = replaceholder(createTemplate(), replaceList());
    fw.setFilePath(dir.filePath("create.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = replaceholder(saveTemplate(), replaceList());
    fw.setFilePath(dir.filePath("save.erb"));
    if (!fw.write(output, false)) {
        return false;
    }
    return true;
}


QString ErbGenerator::indexTemplate() const
{
    return INDEX_TEMPLATE;
}


QString ErbGenerator::showTemplate() const
{
    return SHOW_TEMPLATE;
}


QString ErbGenerator::createTemplate() const
{
    return CREATE_TEMPLATE;
}


QString ErbGenerator::saveTemplate() const
{
    return SAVE_TEMPLATE;
}