/* Copyright (c) 2010-2013, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QHash>
#include <TPrototypeAjaxHelper>
#include <TActionView>

typedef QHash<int, QString> IntHash;

/*!
  \class TPrototypeAjaxHelper
  \brief The TPrototypeAjaxHelper provides some functionality for
  Ajax of Prototype JavaScript framework.
*/


Q_GLOBAL_STATIC_WITH_INITIALIZER(IntHash, behaviorHash,
{
    x->insert(TPrototypeAjaxHelper::InsertBefore,            QLatin1String("insertion:'before', "));
    x->insert(TPrototypeAjaxHelper::InsertAfter,             QLatin1String("insertion:'after', "));
    x->insert(TPrototypeAjaxHelper::InsertAtTopOfContent,    QLatin1String("insertion:'top', "));
    x->insert(TPrototypeAjaxHelper::InsertAtBottomOfContent, QLatin1String("insertion:'bottom', ")); 
})


Q_GLOBAL_STATIC_WITH_INITIALIZER(IntHash, eventStringHash,
{
    x->insert(Tf::Create,        QLatin1String("onCreate:"));
    x->insert(Tf::Uninitialized, QLatin1String("onUninitialized:"));
    x->insert(Tf::Loading,       QLatin1String("onLoading:"));
    x->insert(Tf::Loaded,        QLatin1String("onLoaded:"));
    x->insert(Tf::Interactive,   QLatin1String("onInteractive:"));
    x->insert(Tf::Success,       QLatin1String("onSuccess:"));
    x->insert(Tf::Failure,       QLatin1String("onFailure:"));
    x->insert(Tf::Complete,      QLatin1String("onComplete:"));
})


Q_GLOBAL_STATIC_WITH_INITIALIZER(IntHash, stringOptionHash,
{
    x->insert(Tf::ContentType,    QLatin1String("contentType:"));
    x->insert(Tf::Encoding,       QLatin1String("encoding:"));
    x->insert(Tf::PostBody,       QLatin1String("postBody:"));
    x->insert(Tf::RequestHeaders, QLatin1String("requestHeaders:"));
})

                                 
Q_GLOBAL_STATIC_WITH_INITIALIZER(IntHash, boolOptionHash,
{
    x->insert(Tf::Asynchronous, QLatin1String("asynchronous:"));
    x->insert(Tf::EvalJS,       QLatin1String("evalJS:"));
    x->insert(Tf::EvalJSON,     QLatin1String("evalJSON:"));
    x->insert(Tf::SanitizeJSON, QLatin1String("sanitizeJSON:"));
})


Q_GLOBAL_STATIC_WITH_INITIALIZER(IntHash, methodHash,
{
    x->insert(Tf::Get,     QLatin1String("get"));
    x->insert(Tf::Head,    QLatin1String("head"));
    x->insert(Tf::Post,    QLatin1String("post"));
    x->insert(Tf::Options, QLatin1String("options"));
    x->insert(Tf::Put,     QLatin1String("put"));
    x->insert(Tf::Delete,  QLatin1String("delete"));
    x->insert(Tf::Trace,   QLatin1String("trace"));
})


QString TPrototypeAjaxHelper::requestFunction(const QUrl &url, const TOption &options, const QString &jsCondition) const
{
    QString string;

    if (!jsCondition.isEmpty()) {
        string.append("if (").append(jsCondition).append(") { ");
    }

    string += QLatin1String("new Ajax.Request(\'");
    string += url.toString();
    string += QLatin1String("', { ");
    string += optionsToString(options);
    string += QLatin1String(" });");

    if (!jsCondition.isEmpty()) {
        string += QLatin1String(" }");
    }
    return string;
}


QString TPrototypeAjaxHelper::updateFunction(const QUrl &url, const QString &id, UpdateBehavior behavior, const TOption &options, bool evalScripts, const QString &jsCondition) const
{
    QString string;

    if (!jsCondition.isEmpty()) {
        string.append("if (").append(jsCondition).append(") { ");
    }

    string += QLatin1String("new Ajax.Updater(\'");
    string += id;
    string += QLatin1String("', '");
    string += url.toString();
    string += QLatin1String("', { ");

    // Appends 'insertion' parameter
    if (behavior != Replace) {
        string += behaviorHash()->value(behavior);
    }

    // Appends ajax options
    string += optionsToString(options);

    string += QLatin1String(", evalScripts:");
    string += (evalScripts) ? QLatin1String("true") : QLatin1String("false");
    string += QLatin1String(" });");

    if (!jsCondition.isEmpty()) {
        string += QLatin1String(" }");
    }
    return string; 
}


QString TPrototypeAjaxHelper::periodicalUpdateFunction(const QUrl &url, const QString &id, UpdateBehavior behavior, const TOption &options, bool evalScripts, int frequency, int decay, const QString &jsCondition) const
{
    QString string;

    if (!jsCondition.isEmpty()) {
        string.append("if (").append(jsCondition).append(") { ");
    }

    string += QLatin1String("new Ajax.PeriodicalUpdater(\'");
    string += id;
    string += QLatin1String("', '");
    string += url.toString();
    string += QLatin1String("', { ");
   
    // Appends 'insertion' parameter
    if (behavior != Replace) {
        string += behaviorHash()->value(behavior);
    }

    // Appends ajax options
    string += optionsToString(options);

    string += QLatin1String(", evalScripts:");
    string += (evalScripts) ? QLatin1String("true") : QLatin1String("false");
    string += QLatin1String(", frequency:");
    string += QString::number(frequency);
    string += QLatin1String(", decay:");
    string += QString::number(decay);
    string += QLatin1String(" });");

    if (!jsCondition.isEmpty()) {
        string += QLatin1String(" }");
    }
    return string;
}


QString TPrototypeAjaxHelper::linkToRequest(const QString &text, const QUrl &url, const TOption &options, const QString &jsCondition, const THtmlAttribute &attributes) const
{
    QString string;
    string += QLatin1String("<a href=\"#\" onclick=\"");
    string += requestFunction(url, options, jsCondition);
    string += QLatin1String(" return false;\"");
    string += attributes.toString();
    string += QLatin1Char('>');
    string += text;
    string += QLatin1String("</a>");
    return string;
}


QString TPrototypeAjaxHelper::linkToUpdate(const QString &text, const QUrl &url, const QString &id, UpdateBehavior behavior, const TOption &options, bool evalScripts, const QString &jsCondition, const THtmlAttribute &attributes) const
{
    QString string;
    string += QLatin1String("<a href=\"#\" onclick=\"");
    string += updateFunction(url, id, behavior, options, evalScripts, jsCondition);
    string += QLatin1String(" return false;\"");
    string += attributes.toString();
    string += QLatin1Char('>');
    string += text;
    string += QLatin1String("</a>");
    return string;
}


QString TPrototypeAjaxHelper::linkToPeriodicalUpdate(const QString &text, const QUrl &url, const QString &id, UpdateBehavior behavior, const TOption &options, bool evalScripts, int frequency, int decay, const QString &jsCondition, const THtmlAttribute &attributes) const
{
   QString string;
    string += QLatin1String("<a href=\"#\" onclick=\"");
    string += periodicalUpdateFunction(url, id, behavior, options, evalScripts, frequency, decay, jsCondition);
    string += QLatin1String(" return false;\"");
    string += attributes.toString();
    string += QLatin1Char('>');
    string += text;
    string += QLatin1String("</a>");
    return string;
}


QString TPrototypeAjaxHelper::optionsToString(const TOption &options) const
{
    QString string;

    // Adds authenticity_token
    TOption opt(options);
    QVariantMap map;
    QVariant v = opt[Tf::Parameters];
    if (v.isValid() && v.canConvert(QVariant::Map)) {
        map = v.toMap();
    }
    map.insert("authenticity_token", actionView()->authenticityToken());
    opt.insert(Tf::Parameters, map);

    for (QMapIterator<int, QVariant> i(opt); i.hasNext(); ) {
        i.next();

        // Appends ajax option
        QString s = stringOptionHash()->value(i.key());
        if (!s.isEmpty() && i.value().canConvert(QVariant::String)) {
            string += s;
            string += QLatin1Char('\'');
            string += i.value().toString();
            string += QLatin1String("', ");
            continue;
        }

        s = boolOptionHash()->value(i.key());
        if (!s.isEmpty() && i.value().canConvert(QVariant::Bool)) {
            string += s;
            string += (i.value().toBool()) ? QLatin1String("true, ") : QLatin1String("false, ");
            continue;
        }

        if (i.key() == Tf::Method && i.value().canConvert(QVariant::Int)) {
            string += QLatin1String("method:'");
            string += methodHash()->value(i.value().toInt());
            string += QLatin1String("', ");
            continue;
        }

        // Appends 'parameters' option
        if (i.key() == Tf::Parameters) {
            QString val;
            if (i.value().canConvert(QVariant::Map)) {
                QVariantMap m = i.value().toMap();
                for (QMapIterator<QString, QVariant> it(m); it.hasNext(); ) {
                    it.next();
                    if (it.value().canConvert<TJavaScriptObject>()) {
                        val += it.key();
                        val += QLatin1String(":");
                        val += it.value().value<TJavaScriptObject>().toString();
                        val += QLatin1String(", ");
                    } else if (it.value().canConvert(QVariant::String)) {
                        val += it.key();
                        val += QLatin1String(":'");
                        val += THttpUtility::toUrlEncoding(it.value().toString());
                        val += QLatin1String("', ");
                    }
                }
                val.chop(2);
            }
            
            if (!val.isEmpty()) {
                string += QLatin1String("parameters: { ");
                string += val;
                string += QLatin1String(" }, ");
            }
            continue;
        }

        // Appends ajax callbacks
        s = eventStringHash()->value(i.key());
        if (!s.isEmpty() && i.value().canConvert(QVariant::String)) {
            string += s;
            string += i.value().toString();
            string += QLatin1String(", ");
            continue;
        } else {
            tWarn("invalid parameter: %d [%s:%d]", i.key(), __FILE__, __LINE__);
        }
    }

    string.chop(2);
    return string;
}
