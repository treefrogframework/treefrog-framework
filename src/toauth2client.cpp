#include "toauth2client.h"
#include "thttpclient.h"
#include "thttputility.h"
#include <THttpRequest>
#include <QMap>


class OAuth2ErrorCode : public QMap<QString, int>
{
public:
    OAuth2ErrorCode() : QMap<QString, int>()
    {
        insert(QString("invalid_request"),              TOAuth2Client::InvalidRequest);
        insert(QString("invalid_client"),               TOAuth2Client::InvalidClient);
        insert(QString("invalid_grant"),                TOAuth2Client::InvalidGrant);
        insert(QString("unauthorized_client"),          TOAuth2Client::UnauthorizedClient);
        insert(QString("unsupported_grant_type"),       TOAuth2Client::UnsupportedGrantType);
        insert(QString("access_denied"),                TOAuth2Client::AccessDenied);
        insert(QString("unsupported_response_type"),    TOAuth2Client::UnsupportedResponseType);
        insert(QString("invalid_scope"),                TOAuth2Client::InvalidScope);
        insert(QString("server_error"),                 TOAuth2Client::ServerError);
        insert(QString("temporarily_unavailable"),      TOAuth2Client::TemporarilyUnavailable);
    }
};
Q_GLOBAL_STATIC(OAuth2ErrorCode, oauth2ErrorCode);


TOAuth2Client::TOAuth2Client(const QString &clientId, const QString &clientSecret) :
    _clientId(clientId),
    _clientSecret(clientSecret)
{ }


QUrl TOAuth2Client::startAuthorization(const QUrl &requestUrl, const QStringList &scopes, const QString &state, const QUrl &redirect, int msecs)
{
    THttpClient client;
    QUrl url = requestUrl;
    QString querystr;

    querystr += "response_type=code";
    if (!_clientId.isEmpty()) {
        querystr  = "&client_id=" + _clientId;
    }
    if (!scopes.isEmpty()) {
        querystr += "&scope=" + scopes.join(" ");
    }
    if (!redirect.isEmpty()) {
        querystr += "&redirect_uri=" + redirect.toString(QUrl::None);
    }
    if (!state.isEmpty()) {
        querystr += "&state=" + state;
    }
    url.setQuery(querystr);
    tInfo() << "query:" << url.toEncoded();

    auto *reply = client.get(url, msecs);
    _networkError = reply->error();
    QByteArray location = reply->rawHeader("Location");
    return QUrl(THttpUtility::fromUrlEncoding(location));
}


QString TOAuth2Client::requestAccessToken(const QUrl &requestUrl, const QString &code, int msecs)
{
    QString token;
    THttpClient client;
    QByteArray query;
    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    query  = "client_id=" + THttpUtility::toUrlEncoding(_clientId);
    query += "&client_secret=" + THttpUtility::toUrlEncoding(_clientSecret);
    query += "&code=" + THttpUtility::toUrlEncoding(code);

    auto *reply = client.post(request, query, msecs);
    _networkError = reply->error();
    QByteArray body = reply->readAll();
    auto params = THttpUtility::fromFormUrlEncoded(body);
    for (auto &p : params) {
        tInfo() << p.first << ":" << p.second;
        if (p.first == "access_token") {
            token = p.second;
        }
    }
    return token;
}
