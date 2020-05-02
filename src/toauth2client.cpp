#include "toauth2client.h"
#include "thttpclient.h"
#include "thttputility.h"
#include <QMap>
#include <THttpRequest>

/*!
  \class TOAuth2Client
  \brief The TOAuth2Client class provides an implementation of the
  Authorization Code Grant flow in OAuth2 authentication methods.
  \sa https://tools.ietf.org/html/rfc6749
*/

class OAuth2ErrorCode : public QMap<QString, int> {
public:
    OAuth2ErrorCode() :
        QMap<QString, int>()
    {
        insert(QString("invalid_request"), TOAuth2Client::InvalidRequest);
        insert(QString("invalid_client"), TOAuth2Client::InvalidClient);
        insert(QString("invalid_grant"), TOAuth2Client::InvalidGrant);
        insert(QString("unauthorized_client"), TOAuth2Client::UnauthorizedClient);
        insert(QString("unsupported_grant_type"), TOAuth2Client::UnsupportedGrantType);
        insert(QString("access_denied"), TOAuth2Client::AccessDenied);
        insert(QString("unsupported_response_type"), TOAuth2Client::UnsupportedResponseType);
        insert(QString("invalid_scope"), TOAuth2Client::InvalidScope);
        insert(QString("server_error"), TOAuth2Client::ServerError);
        insert(QString("temporarily_unavailable"), TOAuth2Client::TemporarilyUnavailable);
    }
};
Q_GLOBAL_STATIC(OAuth2ErrorCode, oauth2ErrorCode);


TOAuth2Client::TOAuth2Client(const QString &clientId, const QString &clientSecret) :
    _clientId(clientId),
    _clientSecret(clientSecret)
{
}

/*!
  Initiates the flow by directing the resource owner's user-agent to the
  authorization endpoint of \a requestUrl and returns a QURL object to
  redirect the user-agent.
 */
QUrl TOAuth2Client::startAuthorization(const QUrl &requestUrl, const QStringList &scopes, const QString &state, const QUrl &redirect, const QVariantMap &parameters, int msecs)
{
    THttpClient client;
    QString querystr;
    QUrl url = requestUrl;

    querystr += QLatin1String("response_type=code");
    querystr += QLatin1String("&client_id=");
    querystr += _clientId;

    if (!scopes.isEmpty()) {
        querystr += QLatin1String("&scope=");
        querystr += scopes.join(" ");
    }

    if (!redirect.isEmpty()) {
        querystr += QLatin1String("&redirect_uri=");
        querystr += redirect.toString(QUrl::None);
    }

    if (!state.isEmpty()) {
        querystr += QLatin1String("&state=");
        querystr += state;
    }

    for (auto it = parameters.begin(); it != parameters.end(); ++it) {
        querystr += QLatin1Char('&');
        querystr += it.key();
        querystr += QLatin1Char('=');
        querystr += it.value().toString();
    }
    url.setQuery(querystr);

    auto *reply = client.get(url, msecs);
    _networkError = reply->error();
    QByteArray location = reply->rawHeader("Location");

    auto query = QUrl(location).query(QUrl::FullyDecoded).toLatin1();
    auto params = THttpUtility::fromFormUrlEncoded(query);
    for (auto &p : params) {
        if (p.first == QStringLiteral("error")) {
            _errorCode = (ErrorCode)oauth2ErrorCode()->value(p.second, TOAuth2Client::UnknownError);
        }

        if (p.first == QStringLiteral("error_description")) {
            tError() << "OAuth2 error response. error_description:" << p.second;
        }
    }
    return QUrl(THttpUtility::fromUrlEncoding(location));
}

/*!
  Requests an access token from the authorization server's token endpoint of
  \a requestUrl by including the authorization \a code received.
 */
QString TOAuth2Client::requestAccessToken(const QUrl &requestUrl, const QString &code, const QVariantMap &parameters, int msecs)
{
    QString token;
    THttpClient client;
    QByteArray query;
    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    query += "grant_type=authorization_code";
    query += "&code=";
    query += THttpUtility::toUrlEncoding(code);
    query += "&client_id=";
    query += THttpUtility::toUrlEncoding(_clientId);

    if (!_clientSecret.isEmpty()) {
        query += "&client_secret=";
        query += THttpUtility::toUrlEncoding(_clientSecret);
    }

    for (auto it = parameters.begin(); it != parameters.end(); ++it) {
        query += '&';
        query += it.key().toLatin1();
        query += '=';
        query += THttpUtility::toUrlEncoding(it.value().toString());
    }

    auto *reply = client.post(request, query, msecs);
    _networkError = reply->error();
    QByteArray body = reply->readAll();

    auto params = THttpUtility::fromFormUrlEncoded(body);
    for (auto &p : params) {
        if (p.first == QStringLiteral("access_token")) {
            token = p.second;
        }

        if (p.first == QStringLiteral("error")) {
            _errorCode = (ErrorCode)oauth2ErrorCode()->value(p.second, TOAuth2Client::UnknownError);
        }

        if (p.first == QStringLiteral("error_description")) {
            tError() << "OAuth2 error response. error_description:" << p.second;
        }
    }
    return token;
}
