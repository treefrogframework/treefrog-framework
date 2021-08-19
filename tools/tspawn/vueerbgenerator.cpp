/* Copyright (c) 2012-2021, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "vueerbgenerator.h"
#include "filewriter.h"
#include "global.h"
#include "util.h"
#include <QList>
#include <QPair>


constexpr auto INDEX_TEMPLATE =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "  <meta charset=\"UTF-8\">\n"
    "  <title>%caption%: index</title>\n"
    "  <script src=\"https://unpkg.com/vue@3\"></script>\n"
    "</head>\n"
    "<body>\n"
    "<div id=\"app\"></div>\n"
    "\n"
    "<!-- vue template -->\n"
    "<script type=\"text/x-template\" id=\"%varname%-index-template\">\n"
    "  <h1>Listing %caption%</h1>\n"
    "  <p><a href=\"/%varname%/create\">Create a new %caption%</a></p>\n"
    "  <table border=\"1\" cellpadding=\"5\" style=\"border: 1px #d0d0d0 solid; border-collapse: collapse;\">\n"
    "    <tr>\n"
    "%th%"
    "    </tr>\n"
    "    <tr v-for=\"item in items\">\n"
    "%td%"
    "      <td>\n"
    "        <a :href=\"`/%varname%/show/${item.%pkVarName%}`\">Show</a> <a :href=\"`/%varname%/save/${item.%pkVarName%}`\">Edit</a> <a href=\"#\" @click.prevent=\"postRemove(`/%varname%/remove/${item.%pkVarName%}`)\">Remove</a>\n"
    "      </td>\n"
    "    </tr>\n"
    "  </table>\n"
    "</script>\n"
    "\n"
    "<!-- vue app main -->\n"
    "<script>\n"
    "Vue.createApp({\n"
    "  setup() {\n"
    "    return {\n"
    "      items: <%==$ items %|% \"{}\" %>\n"
    "    };\n"
    "  },\n"
    "  template: \"#%varname%-index-template\",\n"
    "  methods: {\n"
    "    postRemove: function(url) {\n"
    "      if (confirm('Are you sure?')) {\n"
    "        var f = document.createElement('form');\n"
    "        document.body.appendChild(f);\n"
    "        f.method = 'post';\n"
    "        f.action = url;\n"
    "        var i = document.createElement('input');\n"
    "        f.appendChild(i);\n"
    "        i.type = 'hidden';\n"
    "        i.name = 'authenticity_token';\n"
    "        i.value = '<%= authenticityToken() %>';\n"
    "        f.submit();\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "}).mount(\"#app\");\n"
    "</script>\n"
    "</body>\n"
    "</html>\n";


constexpr auto SHOW_TEMPLATE =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "  <meta charset=\"UTF-8\">\n"
    "  <title>%caption%: show</title>\n"
    "  <script src=\"https://unpkg.com/vue@3\"></script>\n"
    "</head>\n"
    "<body>\n"
    "<div id=\"app\"></div>\n"
    "\n"
    "<!-- vue template -->\n"
    "<script type=\"text/x-template\" id=\"%varname%-show-template\">\n"
    "  <h1>Showing %caption%</h1>\n"
    "  <p style=\"color: red\">{{ error }}</p>\n"
    "  <p style=\"color: green\">{{ notice }}</p>\n"
    "%showitems%"
    "  <a :href=\"`/%varname%/save/${item.%pkVarName%}`\">Edit</a> |\n"
    "  <a href=\"/%varname%/index\">Back</a>\n"
    "</script>\n"
    "\n"
    "<!-- vue app main -->\n"
    "<script>\n"
    "Vue.createApp({\n"
    "  setup() {\n"
    "    return {\n"
    "      item: <%==$ item %|% \"{}\" %>,\n"
    "      error: '<%=$ error %>',\n"
    "      notice: '<%=$ notice %>',\n"
    "    };\n"
    "  },\n"
    "  template: \"#%varname%-show-template\",\n"
    "}).mount(\"#app\");\n"
    "</script>\n"
    "</body>\n"
    "</html>\n";


constexpr auto CREATE_TEMPLATE =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "  <meta charset=\"UTF-8\">\n"
    "  <title>%caption%: create</title>\n"
    "  <script src=\"https://unpkg.com/vue@3\"></script>\n"
    "</head>\n"
    "<body>\n"
    "<div id=\"app\"></div>\n"
    "\n"
    "<!-- vue template -->\n"
    "<script type=\"text/x-template\" id=\"%varname%-create-template\">\n"
    "  <h1>New %caption%</h1>\n"
    "  <p style=\"color: red\">{{ error }}</p>\n"
    "  <p style=\"color: green\">{{ notice }}</p>\n"
    "  <form action=\"/%varname%/create\" enctype=\"multipart/form-data\" method=\"post\">\n"
    "    <input type=\"hidden\" name=\"authenticity_token\" :value=\"authenticity_token\" />\n"
    "%entryitems%"
    "    <p>\n"
    "      <input type=\"submit\" value=\"Create\" />\n"
    "    </p>\n"
    "  </form>\n"
    "  <a href=\"/%varname%/index\">Back</a>\n"
    "</script>\n"
    "\n"
    "<!-- vue app main -->\n"
    "<script>\n"
    "Vue.createApp({\n"
    "  setup() {\n"
    "    return {\n"
    "      item: <%==$ item %|% \"{}\" %>,\n"
    "      error: '<%=$ error %>',\n"
    "      notice: '<%=$ notice %>',\n"
    "      authenticity_token: '<%= authenticityToken() %>',\n"
    "    };\n"
    "  },\n"
    "  template: \"#%varname%-create-template\",\n"
    "}).mount(\"#app\");\n"
    "</script>\n"
    "</body>\n"
    "</html>\n";


constexpr auto SAVE_TEMPLATE =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "  <meta charset=\"UTF-8\">\n"
    "  <title>%caption%: save</title>\n"
    "  <script src=\"https://unpkg.com/vue@3\"></script>\n"
    "</head>\n"
    "<body>\n"
    "<div id=\"app\"></div>\n"
    "\n"
    "<!-- vue template -->\n"
    "<script type=\"text/x-template\" id=\"%varname%-save-template\">\n"
    "  <h1>Editing %caption%</h1>\n"
    "  <p style=\"color: red\"><%=$ error %></p>\n"
    "  <p style=\"color: green\"><%=$ notice %></p>\n"
    "  <form :action=\"`/%varname%/save/${item.%pkVarName%}`\" method=\"post\">\n"
    "    <input type=\"hidden\" name=\"authenticity_token\" :value=\"authenticity_token\" />\n"
    "%edititems%"
    "    <p>\n"
    "      <input type=\"submit\" value=\"Save\" />\n"
    "    </p>\n"
    "  </form>\n"
    "  <a :href=\"`/%varname%/show/${item.%pkVarName%}`\">Show</a> |\n"
    "  <a href=\"/%varname%/index\">Back</a>\n"
    "</script>\n"
    "\n"
    "<!-- vue app main -->\n"
    "<script>\n"
    "Vue.createApp({\n"
    "  setup() {\n"
    "    return {\n"
    "      item: <%==$ item %|% \"{}\" %>,\n"
    "      error: '<%=$ error %>',\n"
    "      notice: '<%=$ notice %>',\n"
    "      authenticity_token: '<%= authenticityToken() %>',\n"
    "    };\n"
    "  },\n"
    "  template: \"#%varname%-save-template\",\n"
    "}).mount(\"#app\");\n"
    "</script>\n"
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


VueErbGenerator::VueErbGenerator(const QString &view, const QList<QPair<QString, QMetaType::Type>> &fields, int pkIdx, int autoValIdx) :
    ErbGenerator(view, fields, pkIdx, autoValIdx)
{}


PlaceholderList VueErbGenerator::replaceList() const
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

        showitems += "  <dt>";
        showitems += icap;
        showitems += "</dt><dd>{{ ";
        showitems += "item." + ivar;
        showitems += " }}</dd><br>\n";

        if (!excludedColumn.contains(ivar, Qt::CaseInsensitive)) {
            th += "      <th>";
            th += icap;
            th += "</th>\n";

            td += "      <td>{{ item.";
            td += ivar;
            td += " }}</td>\n";

            if (i != _autoValueIndex) {  // case of not auto-value field
                entryitems += "    <p>\n      <label>";
                entryitems += icap;
                entryitems += "<br><input type=\"text\" name=\"";
                entryitems += varName + '[' + ivar + ']';
                entryitems += "\" :value=\"item.";
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
                edititems += " readonly=\"readonly\"";
            }
            edititems += " /></label>\n    </p>\n";
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


QString VueErbGenerator::indexTemplate() const
{
    return QLatin1String(INDEX_TEMPLATE);
}


QString VueErbGenerator::showTemplate() const
{
    return QLatin1String(SHOW_TEMPLATE);
}


QString VueErbGenerator::createTemplate() const
{
    return QLatin1String(CREATE_TEMPLATE);
}


QString VueErbGenerator::saveTemplate() const
{
    return QLatin1String(SAVE_TEMPLATE);
}