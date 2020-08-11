#pragma once
#include <TGlobal>

class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;
class QJsonDocument;


class T_CORE_EXPORT THttpClient {
public:
    THttpClient();
    ~THttpClient();

    QNetworkReply *get(const QUrl &url, int msecs = 5000);
    QNetworkReply *get(const QNetworkRequest &request, int msecs = 5000);
    QNetworkReply *post(const QUrl &url, const QJsonDocument &json, int msecs = 5000);
    QNetworkReply *post(const QNetworkRequest &request, const QByteArray &data, int msecs = 5000);
    QNetworkReply *put(const QUrl &url, const QJsonDocument &json, int msecs = 5000);
    QNetworkReply *put(const QNetworkRequest &request, const QByteArray &data, int msecs = 5000);
    QNetworkReply *deleteResource(const QUrl &url, int msecs = 5000);
    QNetworkReply *deleteResource(const QNetworkRequest &request, int msecs = 5000);

    QNetworkAccessManager *manager() { return _manager; }

private:
    QNetworkAccessManager *_manager {nullptr};
};

