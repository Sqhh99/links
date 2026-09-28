#ifndef AUTHBACKEND_H
#define AUTHBACKEND_H

#include <QObject>
#include <QString>
#include "network_client.h"

class AuthBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY isLoggedInChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    // accountName is the login username; userName is the name shown to others.
    Q_PROPERTY(QString accountName READ accountName NOTIFY accountNameChanged)
    Q_PROPERTY(QString userName READ userName NOTIFY userNameChanged)
    Q_PROPERTY(QString authToken READ authToken NOTIFY authTokenChanged)

public:
    explicit AuthBackend(QObject* parent = nullptr);
    ~AuthBackend() override = default;

    bool loading() const { return loading_; }
    bool isLoggedIn() const { return isLoggedIn_; }
    QString errorMessage() const { return errorMessage_; }
    QString accountName() const { return accountName_; }
    QString userName() const { return userName_; }
    QString authToken() const { return authToken_; }

    // Signs in, or creates the account when the username is not taken yet.
    Q_INVOKABLE void login(const QString& username, const QString& password);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void switchUser();
    Q_INVOKABLE void tryAutoLogin();

signals:
    void loadingChanged();
    void isLoggedInChanged();
    void errorMessageChanged();
    void accountNameChanged();
    void userNameChanged();
    void authTokenChanged();
    
    void loginSucceeded();
    void switchUserRequested();
    void sessionExpired(const QString& message);
    void authFailed(const QString& error);

private slots:
    void onLoginSuccess(const QString& userId, const QString& username, const QString& token,
                        const QString& displayName);
    void onAuthRefreshed(const QString& userId, const QString& username, const QString& token,
                         const QString& displayName, int expiresInSecs);
    void onAuthExpired(const QString& message);
    void onAuthError(const QString& error);

private:
    void setLoading(bool loading);
    void setErrorMessage(const QString& message);
    void setLoggedIn(bool loggedIn);
    void setAccountName(const QString& username);
    void setUserName(const QString& name);
    void setAuthToken(const QString& token);

    NetworkClient* networkClient_;
    bool loading_{false};
    bool isLoggedIn_{false};
    QString errorMessage_;
    QString accountName_;
    QString userName_;
    QString authToken_;
};

#endif // AUTHBACKEND_H
