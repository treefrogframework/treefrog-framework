#ifndef TOAUTH2CLIENT_H
#define TOAUTH2CLIENT_H

#include <TGlobal>
#include <QStringList>
#include <QUrl>
#include <QNetworkReply>


// class T_CORE_EXPORT TOAuth2AccessToken
// {
// public:
//     QString token;
//     int expires {0};
//     QString refreshToken;
//     QStringList scopes;

//     TOAuth2AccessToken() {}
//     TOAuth2AccessToken(const TOAuth2AccessToken &other) = default;
//     TOAuth2AccessToken &operator=(const TOAuth2AccessToken &other) = default;
// };


class T_CORE_EXPORT TOAuth2Client
{
public:
    enum Error {
        NoError = 0,
        InvalidRequest,
        InvalidClient,
        InvalidGrant,
        UnauthorizedClient,
        UnsupportedGrantType,
        AccessDenied,
        UnsupportedResponseType,
        InvalidScope,
        ServerError,
        TemporarilyUnavailable,
        UnknownError,
    };

    TOAuth2Client(const QString &clientId, const QString &clientSecret = QString());
    TOAuth2Client(const TOAuth2Client &other) = default;
    TOAuth2Client &operator=(const TOAuth2Client &other) = default;

    QUrl startAuthorization(const QUrl &requestUrl, const QStringList &scopes, const QString &state, const QUrl &redirect, int msecs = 5000);
    QString requestAccessToken(const QUrl &requestUrl, const QString &code, int msecs = 5000);
    QString accessToken() const { return _accessToken; }
    int tokenExpires() const { return _expires; }
    //QStringList scopes() const { return _scopes; }
    Error errorCode() { return _error; }
    QNetworkReply::NetworkError networkError() const { return _networkError; }

private:
    QString _clientId;
    QString _clientSecret;
    QString _accessToken;
    int _expires {0};
    Error _error {NoError};
    QNetworkReply::NetworkError _networkError {QNetworkReply::NoError};
};

#endif // TOAUTH2CLIENT_H
