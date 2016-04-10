#ifndef TJSCONTEXT_H
#define TJSCONTEXT_H

#include <QString>
#include <QStringList>
#include <QJSValue>
#include <QDir>
#include <QMutex>
#include <TGlobal>

class QJSEngine;


class T_CORE_EXPORT TJSContext
{
public:
    TJSContext(bool commonJsMode = false, const QStringList &scriptFiles = QStringList());
    virtual ~TJSContext();

    bool load(const QString &moduleName, const QDir &dir = QDir("."));
    QJSValue evaluate(const QString &program, const QString &fileName = QString(), int lineNumber = 1);
    QJSValue call(const QString &func, const QJSValue &arg);
    QJSValue call(const QString &func, const QJSValueList &args = QJSValueList());

protected:
    QString read(const QString &moduleName, const QDir &dir = QDir("."));
    void replaceRequire(QString &content, const QDir &dir);

private:
    QJSEngine *jsEngine;
    bool commonJs;
    QJSValue *funcObj;
    QString lastFunc;
    QMutex mutex;

    Q_DISABLE_COPY(TJSContext);
};

#endif // TJSCONTEXT_H
