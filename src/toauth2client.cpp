#include "toauth2client.h"
#include "thttpclient.h"
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


bool TOAuth2Client::requestAccessToken(const QUrl &authorizeUrl, const QString &code, const QStringList &scopes, const QUrl &redirect, int msecs)
{
    _authorizeUrl = authorizeUrl;
    _code = code;
    _scopes = scopes;
    _redirect = redirect;

    THttpClient client;
    QUrl url = authorizeUrl;
    QString querystr;

    querystr  = "client_id=" + _clientId;
    querystr += "&scope=" + scopes.join(" ");
    querystr += "&redirect_uri=" + redirect.toString(QUrl::None);
    querystr += "&response_type=code";
    url.setQuery(querystr);
    tInfo() << "query:" << url.toEncoded();

    auto *reply = client.get(url, msecs);
    _networkError = reply->error();
    auto location = reply->rawHeader("Location");
    if (location.isEmpty()) {
        // error
        return false;
    }
    tInfo() << location;
    return true;
}
