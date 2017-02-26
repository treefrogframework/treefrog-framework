/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "otamagenerator.h"
#include "global.h"
#include "projectfilegenerator.h"
#include "filewriter.h"
#include "util.h"

#define INDEX_HTML_TEMPLATE                                             \
    "<!DOCTYPE html>\n"                                                 \
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
    "for (const auto &i : %3List) {\n"                                  \
    "    %%\n"                                                          \
    "}\n"                                                               \
    "\n"                                                                \
    "%4"                                                                \
    "@link_to_show :== linkTo(\"Show\", urla(\"show\", QStringList()%5))\n"      \
    "\n"                                                                \
    "@link_to_edit :== linkTo(\"Edit\", urla(\"save\", QStringList()%5))\n"      \
    "\n"                                                                \
    "@link_to_remove :== linkTo(\"Remove\", urla(\"remove\", QStringList()%5), Tf::Post, \"confirm('Are you sure?')\")\n" \
    "\n"                                                                \
    "@link_to_entry :== linkTo(\"New entry\", urla(\"create\"))\n"

#define SHOW_HTML_TEMPLATE                                              \
    "<!DOCTYPE html>\n"                                                 \
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
    "@link_to_edit :== linkTo(\"Edit\", urla(\"save\", QStringList()%5))\n"     \
    "\n"                                                                \
    "@link_to_index :== linkTo(\"Back\", urla(\"index\"))\n"

#define CREATE_HTML_TEMPLATE                                            \
    "<!DOCTYPE html>\n"                                                 \
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

#define CREATE_OTM_TEMPLATE                                             \
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

#define SAVE_HTML_TEMPLATE                                              \
    "<!DOCTYPE html>\n"                                                 \
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

#define SAVE_OTM_TEMPLATE                                               \
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
    "@edit_form |== formTag(urla(\"save\", QStringList()%3))\n"              \
    "\n"                                                                \
    "%4"                                                                \
    "@link_to_show |== linkTo(\"Show\", urla(\"show\", QStringList()%3))\n"  \
    "\n"                                                                \
    "@link_to_index |== linkTo(\"Back\", urla(\"index\"))\n"


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


OtamaGenerator::OtamaGenerator(const QString &view, const QList<QPair<QString, QVariant::Type>> &fields, QList<int> pkIdxs, int autoValIdx)
    : viewName(view), fieldList(fields), primaryKeyIndexs(pkIdxs), autoValueIndex(autoValIdx)
{ }


bool OtamaGenerator::generate(const QString &dstDir) const
{
    QDir dir(dstDir + viewName.toLower());

    // Reserved word check
    if (excludedDirName.contains(dir.dirName())) {
        qCritical("Reserved word error. Please use another word.  View name: %s", qPrintable(dir.dirName()));
        return false;
    }

    mkpath(dir);
    copy(dataDirPath + ".trim_mode", dir);

    // Generates view files
    generateViews(dir.path());
    return true;
}


QStringList OtamaGenerator::generateViews(const QString &dstDir) const
{
    QStringList files;

    if (primaryKeyIndexs.isEmpty()) {
        qWarning("Primary key not found. [view name: %s]", qPrintable(viewName));
        return files;
    }

    QDir dir(dstDir);
    FileWriter fw;
    QString output;
    QString caption = enumNameToCaption(viewName);
    QString varName = enumNameToVariableName(viewName);

    QStringList pkVarNames;
    QString indexPkValues,showPkValues,savePkValues;
    for (auto &p :primaryKeyIndexs){
        QString pkVarName = fieldNameToVariableName(fieldList[p].first);
        pkVarNames<<pkVarName;
        switch (fieldList[p].second) {
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::LongLong:
        case QVariant::ULongLong:
        case QVariant::Double:
            indexPkValues += QString("<<QString::number(i.%1())").arg(pkVarName);
            showPkValues += QString("<<QString::number(%1.%2())").arg(varName,pkVarName);
            savePkValues += QString("<<%1[\"%2\"].toString()").arg(varName,pkVarName);
            break;
        default:
            indexPkValues += QString("<<i.%1()").arg(pkVarName);
            showPkValues += QString("<<%1.%2()").arg(varName,pkVarName);
            savePkValues += QString("<<%1[\"%2\"].toString()").arg(varName,pkVarName);
            break;
        }
    }

    // Generates index.html
    QString th ,td, indexOtm, showColumn, showOtm, entryColumn, editColumn, entryOtm, editOtm;
    for (int i = 0; i < fieldList.count(); ++i) {
        const QPair<QString, QVariant::Type> &p = fieldList[i];
        QString cap = fieldNameToCaption(p.first);
        QString var = fieldNameToVariableName(p.first);
        QString mrk = p.first.toLower();
        QString readonly;

        if (!excludedColumn.contains(var, Qt::CaseInsensitive)) {
            th += "    <th>";
            th += cap;
            th += "</th>\n";

            td += "    <td data-tf=\"@";
            td += mrk;
            td += "\"></td>\n";

            indexOtm += QString("@%1 ~= i.%2()\n\n").arg(mrk, var);

            if (i != autoValueIndex) {  // case of not auto-value field
                entryColumn += QString("  <p>\n    <label>%1<br /><input data-tf=\"@%2\" /></label>\n  </p>\n").arg(cap, mrk);
                entryOtm += QString("@%1 |== inputTextTag(\"%2[%3]\", %2[\"%3\"].toString())\n\n").arg(mrk, varName, var);
            }

            editColumn += QString("  <p>\n    <label>%1<br /><input data-tf=\"@%2\" /></label>\n  </p>\n").arg(cap, mrk);
            if  (pkVarNames.contains(p.first)) {
                readonly = QLatin1String(", a(\"readonly\", \"readonly\")");
            }
            editOtm += QString("@%1 |== inputTextTag(\"%2[%3]\", %2[\"%3\"].toString()%4);\n\n").arg(mrk, varName, var, readonly);
        }
        showColumn += QString("<dt>%1</dt><dd data-tf=\"@%2\">(%3)</dd><br />\n").arg(cap, mrk, var);
        showOtm += QString("@%1 ~= %2.%3()\n\n").arg(mrk, varName, var);
    }

    output = QString(INDEX_HTML_TEMPLATE).arg(caption, th, td);
    fw.setFilePath(dir.filePath("index.html"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates index.otm

    output = QString(INDEX_OTM_TEMPLATE).arg(varName.toLower(), viewName, varName, indexOtm, indexPkValues);
    fw.setFilePath(dir.filePath("index.otm"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates show.html
    output = QString(SHOW_HTML_TEMPLATE).arg(caption, showColumn);
    fw.setFilePath(dir.filePath("show.html"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates show.otm
    output = QString(SHOW_OTM_TEMPLATE).arg(varName.toLower(), viewName, varName, showOtm, showPkValues);
    fw.setFilePath(dir.filePath("show.otm"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates entry.html
    output = QString(CREATE_HTML_TEMPLATE).arg(caption, entryColumn);
    fw.setFilePath(dir.filePath("create.html"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates entry.otm
    output = QString(CREATE_OTM_TEMPLATE).arg(varName.toLower(), varName, entryOtm);
    fw.setFilePath(dir.filePath("create.otm"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates edit.html
    output = QString(SAVE_HTML_TEMPLATE).arg(caption, editColumn);
    fw.setFilePath(dir.filePath("save.html"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates edit.otm
    output = QString(SAVE_OTM_TEMPLATE).arg(varName.toLower(), varName, savePkValues, editOtm);
    fw.setFilePath(dir.filePath("save.otm"));
    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    return files;
}
