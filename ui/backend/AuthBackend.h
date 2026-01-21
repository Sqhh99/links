#ifndef AUTHBACKEND_H
#define AUTHBACKEND_H

#include <QObject>
#include <QString>
#include <QTimer>
#include "../core/network_client.h"

class AuthBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY isLoggedInChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString userEmail READ userEmail NOTIFY userEmailChanged)
    Q_PROPERTY(QString userName READ userName NOTIFY userNameChanged)
    Q_PROPERTY(int codeCooldown READ codeCooldown NOTIFY codeCooldownChanged)

public:
    explicit AuthBackend(QObject* parent = nullptr);
    ~AuthBackend() override = default;

    bool loading() const { return loading_; }
    bool isLoggedIn() const { return isLoggedIn_; }
    QString errorMessage() const { return errorMessage_; }
    QString userEmail() const { return userEmail_; }
    QString userName() const { return userName_; }
    int codeCooldown() const { return codeCooldown_; }

    Q_INVOKABLE void login(const QString& email, const QString& password);
    Q_INVOKABLE void requestCode(const QString& email);
    Q_INVOKABLE void registerUser(const QString& displayName, const QString& email,
                                   const QString& code, const QString& password);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void tryAutoLogin();

signals:
    void loadingChanged();
    void isLoggedInChanged();
    void errorMessageChanged();
    void userEmailChanged();
    void userNameChanged();
    void codeCooldownChanged();
    
    void loginSucceeded();
    void registerSucceeded();
    void codeRequestSucceeded();
    void authFailed(const QString& error);

private slots:
    void onLoginSuccess(const QString& userId, const QString& email, const QString& token);
    void onRegisterSuccess(const QString& userId, const QString& email, const QString& token);
    void onCodeRequestSuccess(int expiresInSecs);
    void onAuthError(const QString& error);
    void onCooldownTick();

private:
    void setLoading(bool loading);
    void setErrorMessage(const QString& message);
    void setLoggedIn(bool loggedIn);
    void setUserEmail(const QString& email);
    void setUserName(const QString& name);
    void startCooldownTimer();

    NetworkClient* networkClient_;
    QTimer* cooldownTimer_;
    bool loading_{false};
    bool isLoggedIn_{false};
    QString errorMessage_;
    QString userEmail_;
    QString userName_;
    QString pendingDisplayName_;
    int codeCooldown_{0};
};

#endif // AUTHBACKEND_H
