#ifndef TREACTCOMPONENT_H
#define TREACTCOMPONENT_H

#include <QString>
#include <QDateTime>
#include <TGlobal>

class TJSContext;
class QJSValue;


class T_CORE_EXPORT TReactComponent
{
public:
    TReactComponent(const QString &scriptFile);
    virtual ~TReactComponent();

    QString filePath() const;
    bool import(const QString &moduleName);
    QString renderToString(const QString &component);
    QDateTime loadedDateTime() const { return loadedTime; }

    static QString compileJsx(const QString &jsx);
    static QString compileJsxFile(const QString &fileName);

protected:
    void init();

private:
    TJSContext *context;
    QJSValue *jsValue;
    QString scriptPath;
    QDateTime loadedTime;

    Q_DISABLE_COPY(TReactComponent)
};

#endif // TREACTCOMPONENT_H
