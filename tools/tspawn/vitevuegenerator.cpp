/* Copyright (c) 2025, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "vitevuegenerator.h"
#include "filewriter.h"
#include "util.h"
#include <QPair>


constexpr auto INDEX_TEMPLATE =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "  <meta charset=\"UTF-8\">\n"
    "  <title>%ClassName%: index</title>\n"
    "<% if (databaseEnvironment() == \"dev\") { %>\n"
    "  <script type=\"module\" src=\"http://localhost:5173/src/main.js\"></script>\n"
    "<% } else { %>\n"
    "  <%== viteScriptTag(\"src/main.js\", a(\"type\", \"module\")) %>\n"
    "<% } %>\n"
    "  <meta name=\"authenticity_token\" content=\"<%= authenticityToken() %>\">\n"
    "</head>\n"
    "<body>\n"
    "<div data-vue-component=\"%ClassName%Index\"></div>\n"
    "<script id=\"%ClassName%Index-props\" type=\"application/json\"><%==$ props %></script>\n"
    "</body>\n"
    "</html>\n";

constexpr auto CREATE_TEMPLATE =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "  <meta charset=\"UTF-8\">\n"
    "  <title>%ClassName%: create</title>\n"
    "<% if (databaseEnvironment() == \"dev\") { %>\n"
    "  <script type=\"module\" src=\"http://localhost:5173/src/main.js\"></script>\n"
    "<% } else { %>\n"
    "  <%== viteScriptTag(\"src/main.js\", a(\"type\", \"module\")) %>\n"
    "<% } %>\n"
    "  <meta name=\"authenticity_token\" content=\"<%= authenticityToken() %>\">\n"
    "</head>\n"
    "<body>\n"
    "<div data-vue-component=\"%ClassName%Create\"></div>\n"
    "<script id=\"%ClassName%Create-props\" type=\"application/json\"><%==$ props %></script>\n"
    "</body>\n"
    "</html>\n";

constexpr auto SHOW_TEMPLATE =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "  <meta charset=\"UTF-8\">\n"
    "  <title>%ClassName%: show</title>\n"
    "<% if (databaseEnvironment() == \"dev\") { %>\n"
    "  <script type=\"module\" src=\"http://localhost:5173/src/main.js\"></script>\n"
    "<% } else { %>\n"
    "  <%== viteScriptTag(\"src/main.js\", a(\"type\", \"module\")) %>\n"
    "<% } %>\n"
    "</head>\n"
    "<body>\n"
    "<div data-vue-component=\"%ClassName%Show\"></div>\n"
    "<script id=\"%ClassName%Show-props\" type=\"application/json\"><%==$ props %></script>\n"
    "</body>\n"
    "</html>\n";

constexpr auto SAVE_TEMPLATE =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "  <meta charset=\"UTF-8\">\n"
    "  <title>%ClassName%: save</title>\n"
    "<% if (databaseEnvironment() == \"dev\") { %>\n"
    "  <script type=\"module\" src=\"http://localhost:5173/src/main.js\"></script>\n"
    "<% } else { %>\n"
    "  <%== viteScriptTag(\"src/main.js\", a(\"type\", \"module\")) %>\n"
    "<% } %>\n"
    "  <meta name=\"authenticity_token\" content=\"<%= authenticityToken() %>\">\n"
    "</head>\n"
    "<body>\n"
    "<div data-vue-component=\"%ClassName%Save\"></div>\n"
    "<script id=\"%ClassName%Save-props\" type=\"application/json\"><%==$ props %></script>\n"
    "</body>\n"
    "</html>\n";

constexpr auto INDEX_VUE_TEMPLATE =
    "<script setup lang=\"ts\">\n"
    "type %ClassName% = {\n"
    "}\n"
    "\n"
    "defineProps<{\n"
    "    items: %ClassName%[]\n"
    "}>()\n"
    "\n"
    "const token: string = document.querySelector('meta[name=\"authenticity_token\"]')?.getAttribute(\"content\") ?? \"\"\n"
    "\n"
    "const postRemove = (url: string) => {\n"
    "    if (confirm(\"Are you sure?\")) {\n"
    "        const f = document.createElement(\"form\")\n"
    "        f.method = \"post\"\n"
    "        f.action = url\n"
    "        const i = document.createElement(\"input\")\n"
    "        i.type = \"hidden\"\n"
    "        i.name = \"authenticity_token\"\n"
    "        i.value = token\n"
    "        f.appendChild(i)\n"
    "        document.body.appendChild(f)\n"
    "        f.submit()\n"
    "    }\n"
    "}\n"
    "</script>\n"
    "\n"
    "<template>\n"
    "  <h1>Listing %caption%</h1>\n"
    "  <p><a href=\"/%varName%/create\">Create a new %caption%</a></p>\n"
    "  <table cellpadding=\"5\">\n"
    "    <thead>\n"
    "    <tr>\n"
    "%th%"
    "    </tr>\n"
    "    </thead>\n"
    "    <tbody>\n"
    "      <tr v-for=\"item in items\">\n"
    "%td%"
    "        <td>\n"
    "          <a :href=\"`/%varName%/show/${item.%pkVarName%}`\">Show</a> <a :href=\"`/%varName%/save/${item.%pkVarName%}`\">Edit</a> <a href=\"#\" @click.prevent=\"postRemove(`/%varName%/remove/${item.%pkVarName%}`)\">Remove</a>\n"
    "        </td>\n"
    "      </tr>\n"
    "    </tbody>\n"
    "  </table>\n"
    "</template>\n"
    "\n"
    "<style scoped>\n"
    "table {\n"
    "  border-collapse: collapse;\n"
    "}\n"
    "  \n"
    "table, th, td {\n"
    "  border: 1px solid gray;\n"
    "}\n"
    "</style>\n";

constexpr auto SHOW_VUE_TEMPLATE =
    "<script setup lang=\"ts\">\n"
    "type %ClassName% = {\n"
    "}\n"
    "\n"
    "defineProps<{\n"
    "    item: %ClassName%\n"
    "    notice: string,\n"
    "    error: string,\n"
    "}>()\n"
    "</script>\n"
    "\n"
    "<template>\n"
    "  <h1>Showing %caption%</h1>\n"
    "  <p style=\"color: green\">{{ notice }}</p>\n"
    "  <p style=\"color: red\">{{ error }}</p>\n"
    "%showitems%"
    "  <br>\n"
    "  <a :href=\"`/%varName%/save/${item.%pkVarName%}`\">Edit</a> |\n"
    "  <a href=\"/%varName%/index\">Back</a>\n"
    "</template>\n"
    "\n"
    "<style scoped>\n"
    "</style>\n";

constexpr auto CREATE_VUE_TEMPLATE =
    "<script setup lang=\"ts\">\n"
    "type %ClassName% = {\n"
    "}\n"
    "\n"
    "defineProps<{\n"
    "    item: %ClassName%\n"
    "    error: string\n"
    "}>()\n"
    "\n"
    "const token = document.querySelector('meta[name=\"authenticity_token\"]')?.getAttribute('content') ?? \"\"\n"
    "</script>\n"
    "\n"
    "<template>\n"
    "  <h1>New %caption%</h1>\n"
    "  <p style=\"color: red\">{{ error }}</p>\n"
    "  <form action=\"/%varName%/create\" enctype=\"multipart/form-data\" method=\"post\">\n"
    "    <input type=\"hidden\" name=\"authenticity_token\" :value=\"token\" />\n"
    "%entryitems%"
    "    <p>\n"
    "      <input type=\"submit\" value=\"Create\" />\n"
    "    </p>\n"
    "  </form>\n"
    "  <a href=\"/%varName%/index\">Back</a>\n"
    "</template>\n"
    "\n"
    "<style scoped>\n"
    "</style>\n";

constexpr auto SAVE_VUE_TEMPLATE =
    "<script setup lang=\"ts\">\n"
    "type %ClassName% = {\n"
    "}\n"
    "\n"
    "defineProps<{\n"
    "    item: %ClassName%\n"
    "    error: string\n"
    "}>()\n"
    "\n"
    "const token: string = document.querySelector('meta[name=\"authenticity_token\"]')?.getAttribute(\"content\") ?? \"\"\n"
    "</script>\n"
    "\n"
    "<template>\n"
    "  <h1>Editing %caption%</h1>\n"
    "  <p style=\"color: red\">{{ error }}</p>\n"
    "  <form :action=\"`/%varName%/save/${item.%pkVarName%}`\" method=\"post\">\n"
    "    <input type=\"hidden\" name=\"authenticity_token\" :value=\"token\" />\n"
    "%edititems%"
    "    <p>\n"
    "      <input type=\"submit\" value=\"Save\" />\n"
    "    </p>\n"
    "  </form>\n"
    "  <a :href=\"`/%varName%/show/${item.%pkVarName%}`\">Show</a> |\n"
    "  <a href=\"/%varName%/index\">Back</a>\n"
    "</template>\n"
    "\n"
    "<style scoped>\n"
    "</style>\n";


const QStringList excludedColumn = {
    "created_at",
    "updated_at",
    "modified_at",
    "lock_revision",
    "createdAt",
    "updatedAt",
    "modifiedAt",
    "lockRevision",
};

namespace {

const QStringList excludedDirName = {
    "layouts",
    "partial",
    "direct",
    "_src",
    "mailer",
};

}


 ViteVueGenerator::ViteVueGenerator(const QString &view, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int autoValIdx) :
    _viewName(view), _fieldList(fields), _primaryKeyIndex(pkIdx), _autoValueIndex(autoValIdx)
{
}


bool ViteVueGenerator::generate(const QString &dstDir) const
{
    QDir dir(dstDir + _viewName.toLower());
    QString varName = enumNameToVariableName(_viewName);
    const QPair<QString, QMetaType::Type> &pkFld = _fieldList[_primaryKeyIndex];
    QString pkVarName = fieldNameToVariableName(pkFld.first);
    QString th, td, showitems, entryitems, edititems;

    // Generates view files
    for (int i = 0; i < _fieldList.count(); ++i) {
        const QPair<QString, QMetaType::Type> &p = _fieldList[i];

        QString icap = fieldNameToCaption(p.first);
        QString ivar = fieldNameToVariableName(p.first);

        showitems += "  <dt>";
        showitems += icap;
        showitems += "</dt><dd>{{ item.";
        showitems += ivar;
        showitems += " }}</dd>\n";

        if (!excludedColumn.contains(ivar, Qt::CaseInsensitive)) {
            th += "      <th>";
            th += icap;
            th += "</th>\n";
            td += "        <td>{{ item.";
            td += ivar;
            td += " }}</td>\n";

            if (i != _autoValueIndex) {  // case of not auto-value field
                entryitems += "    <p>\n      <label>";
                entryitems += icap;
                entryitems += "<br><input name=\"";
                entryitems += varName + '[' + ivar + ']';
                entryitems += "\" :value=\"item?.";
                entryitems += ivar;
                entryitems += "\" /></label>\n    </p>\n";
            }
            edititems += "    <p>\n      <label>";
            edititems += icap;
            edititems += "<br><input type=\"text\" name=\"";
            edititems += varName + '[' + ivar + ']';
            edititems += "\" :value=\"item.";
            edititems += ivar;
            edititems += "\"";

            if (p.first == pkFld.first) {
                edititems += " readonly";
            }
            edititems += " /></label>\n    </p>\n";
        }
    }

    QList<QPair<QString, QString>> replaceList = {
        {"ClassName", _viewName},
        {"caption", enumNameToCaption(_viewName)},
        {"varName", varName},
        {"th", th},
        {"td", td},
        {"pkVarName", pkVarName},
        {"showitems", showitems},
        {"entryitems", entryitems},
        {"edititems", edititems},
    };

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

    output = replaceholder(INDEX_TEMPLATE, replaceList);
    fw.setFilePath(dir.filePath("index.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = replaceholder(SHOW_TEMPLATE, replaceList);
    fw.setFilePath(dir.filePath("show.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = replaceholder(CREATE_TEMPLATE, replaceList);
    fw.setFilePath(dir.filePath("create.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = replaceholder(SAVE_TEMPLATE, replaceList);
    fw.setFilePath(dir.filePath("save.erb"));
    if (!fw.write(output, false)) {
        return false;
    }

    // Vue files
    QDir vueDir(dir.path() + QDir::separator() + ".." + QDir::separator() + ".." + QDir::separator() + "frontend" + QDir::separator() + "src" + QDir::separator() + "components");
    mkpath(vueDir);

    output = replaceholder(INDEX_VUE_TEMPLATE, replaceList);
    fw.setFilePath(vueDir.filePath(_viewName + "Index.vue"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = replaceholder(SHOW_VUE_TEMPLATE, replaceList);
    fw.setFilePath(vueDir.filePath(_viewName + "Show.vue"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = replaceholder(CREATE_VUE_TEMPLATE, replaceList);
    fw.setFilePath(vueDir.filePath(_viewName + "Create.vue"));
    if (!fw.write(output, false)) {
        return false;
    }

    output = replaceholder(SAVE_VUE_TEMPLATE, replaceList);
    fw.setFilePath(vueDir.filePath(_viewName + "Save.vue"));
    if (!fw.write(output, false)) {
        return false;
    }

    return true;
}
