#ifndef TJSCONTEXT_H
#define TJSCONTEXT_H

#include <QString>
#include <QStringList>
#include <QJSValue>
#include <QDir>
#include <QMap>
#include <QMutex>
#include <TGlobal>

class QJSEngine;
class TJSInstance;


class T_CORE_EXPORT TJSContext
{
public:
    TJSContext(const QStringList &moduleNames = QStringList());
    virtual ~TJSContext();

    QJSValue import(const QString &moduleName);
    QJSValue import(const QString &defaultMember, const QString &moduleName);
    QJSValue evaluate(const QString &program, const QString &fileName = QString(), int lineNumber = 1);
    QJSValue call(const QString &func, const QJSValue &arg);
    QJSValue call(const QString &func, const QJSValueList &args = QJSValueList());
    TJSInstance callAsConstructor(const QString &constructorName, const QJSValue &arg);
    TJSInstance callAsConstructor(const QString &constructorName, const QJSValueList &args = QJSValueList());
    QString lastImportedModulePath() const { return importedModulePath; }
    static void setSearchPaths(const QStringList &paths);

protected:
    QString read(const QString &filePath);
    void replaceRequire(QString &content, const QDir &dir);

private:
    QJSEngine *jsEngine;
    QMap<QString, QString> loadedFiles;
    QJSValue *funcObj;
    QString lastFunc;
    QString importedModulePath;
    QMutex mutex;

    Q_DISABLE_COPY(TJSContext);
};

#endif // TJSCONTEXT_H
