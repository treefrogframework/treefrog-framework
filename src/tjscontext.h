#ifndef TJSCONTEXT_H
#define TJSCONTEXT_H

#include <QString>
#include <QJSValue>
#include <QMutex>
#include <TGlobal>

class QJSEngine;


class T_CORE_EXPORT TJSContext
{
public:
    TJSContext(const QStringList &scriptFiles = QStringList());
    virtual ~TJSContext();

    bool load(const QString &scriptFile);
    QJSValue evaluate(const QString &program, const QString &fileName = QString(), int lineNumber = 1);
    QJSValue call(const QString &func, const QJSValue &arg);
    QJSValue call(const QString &func, const QJSValueList &args = QJSValueList());

private:
    QJSEngine *jsEngine;
    QJSValue *funcObj;
    QString lastFunc;
    QMutex mutex;

    Q_DISABLE_COPY(TJSContext);
};

#endif // TJSCONTEXT_H
