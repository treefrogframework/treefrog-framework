#ifndef TACTIONCONTEXT_H
#define TACTIONCONTEXT_H

#include <QStringList>
#include <QMap>
#include <QSqlDatabase>
#include <TGlobal>
#include <TSqlTransaction>
#include <TKvsDatabase>
#include <TAccessLog>

class QHostAddress;
class THttpResponseHeader;
class THttpSocket;
class THttpRequest;
class THttpResponse;
class TApplicationServer;
class TTemporaryFile;
class TActionController;


class T_CORE_EXPORT TActionContext
{
public:
    TActionContext();
    virtual ~TActionContext();

    QSqlDatabase &getSqlDatabase(int id);
    void releaseSqlDatabases();
    TKvsDatabase &getKvsDatabase(TKvsDatabase::Type type);
    void releaseKvsDatabases();
    TTemporaryFile &createTemporaryFile();
    void stop() { stopped = true; }
    QHostAddress clientAddress() const;
    const TActionController *currentController() const { return currController; }
    THttpRequest &httpRequest() { return *httpReq; }
    const THttpRequest &httpRequest() const { return *httpReq; }

    static int contextCount();
    static void releaseAll();

protected:
    void execute(THttpRequest &request);
    void release();

    virtual void emitError(int socketError);
    bool beginTransaction(QSqlDatabase &database);
    void commitTransactions();
    void rollbackTransactions();

    int socketDescriptor() const { return socketDesc; }
    qint64 writeResponse(int statusCode, THttpResponseHeader &header);
    qint64 writeResponse(int statusCode, THttpResponseHeader &header, const QByteArray &contentType, QIODevice *body, qint64 length);
    qint64 writeResponse(THttpResponseHeader &header, QIODevice *body, qint64 length);

    virtual qint64 writeResponse(THttpResponseHeader &, QIODevice *) { return 0; }
    virtual void closeHttpSocket() { }

    TSqlTransaction transactions;
    QMap<int, QSqlDatabase> sqlDatabases;
    QMap<int, TKvsDatabase> kvsDatabases;
    volatile bool stopped;
    QStringList autoRemoveFiles;
    int socketDesc;
    TAccessLogger accessLogger;

private:
    TActionController *currController;
    QList<TTemporaryFile *> tempFiles;
    THttpRequest *httpReq;

    Q_DISABLE_COPY(TActionContext)
};

#endif // TACTIONCONTEXT_H
