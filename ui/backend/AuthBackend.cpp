#include "AuthBackend.h"
#include "../utils/settings.h"
#include "../utils/logger.h"

AuthBackend::AuthBackend(QObject* parent)
    : QObject(parent),
      networkClient_(new NetworkClient(this)),
      cooldownTimer_(new QTimer(this))
{
    // Connect network signals
    connect(networkClient_, &NetworkClient::loginSuccess,
            this, &AuthBackend::onLoginSuccess);
    connect(networkClient_, &NetworkClient::registerSuccess,
            this, &AuthBackend::onRegisterSuccess);
    connect(networkClient_, &NetworkClient::codeRequestSuccess,
            this, &AuthBackend::onCodeRequestSuccess);
    connect(networkClient_, &NetworkClient::authRefreshed,
            this, &AuthBackend::onAuthRefreshed);
    connect(networkClient_, &NetworkClient::authExpired,
            this, &AuthBackend::onAuthExpired);
    connect(networkClient_, &NetworkClient::authError,
            this, &AuthBackend::onAuthError);
    
    // Setup cooldown timer
    cooldownTimer_->setInterval(1000);
    connect(cooldownTimer_, &QTimer::timeout, this, &AuthBackend::onCooldownTick);
    
    // Set API URL from settings
    networkClient_->setApiUrl(Settings::instance().getSignalingServerUrl());
    
    // Try auto-login on creation
    tryAutoLogin();
}

void AuthBackend::login(const QString& email, const QString& password)
{
    if (loading_) return;
    
    setLoading(true);
    setErrorMessage("");
    networkClient_->login(email, password);
}

void AuthBackend::requestCode(const QString& email)
{
    if (loading_ || codeCooldown_ > 0) return;
    
    setLoading(true);
    setErrorMessage("");
    networkClient_->requestVerificationCode(email);
}

void AuthBackend::registerUser(const QString& displayName, const QString& email,
                                const QString& code, const QString& password)
{
    if (loading_) return;

    setLoading(true);
    setErrorMessage("");
    networkClient_->registerUser(email, password, code, displayName.trimmed());
}

void AuthBackend::logout()
{
    Settings::instance().clearAuthData();
    setLoggedIn(false);
    setUserEmail("");
    setUserName("");
    setLoading(false);
    Logger::instance().info("User logged out");
}

void AuthBackend::switchUser()
{
    logout();
    setErrorMessage("");
    emit switchUserRequested();
}

void AuthBackend::tryAutoLogin()
{
    if (Settings::instance().hasAuthData()) {
        setLoading(true);
        setErrorMessage("");
        networkClient_->refreshAuthToken(Settings::instance().getAuthToken());
    }
}

void AuthBackend::onLoginSuccess(const QString& userId, const QString& email, const QString& token,
                                 const QString& displayName)
{
    setLoading(false);
    
    // Save auth data
    Settings::instance().setAuthToken(token);
    Settings::instance().setUserId(userId);
    Settings::instance().setUserEmail(email);
    
    QString resolvedDisplayName = displayName.trimmed();
    if (resolvedDisplayName.isEmpty()) {
        resolvedDisplayName = email.split("@").first();
    }
    Settings::instance().setDisplayName(resolvedDisplayName);

    setUserEmail(email);
    setUserName(resolvedDisplayName);
    setLoggedIn(true);
    
    Logger::instance().info("Login successful, user: " + email);
    emit loginSucceeded();
}

void AuthBackend::onRegisterSuccess(const QString& userId, const QString& email, const QString& token,
                                    const QString& displayName)
{
    setLoading(false);
    
    // Save auth data
    Settings::instance().setAuthToken(token);
    Settings::instance().setUserId(userId);
    Settings::instance().setUserEmail(email);
    
    QString resolvedDisplayName = displayName.trimmed();
    if (resolvedDisplayName.isEmpty()) {
        resolvedDisplayName = email.split("@").first();
    }
    Settings::instance().setDisplayName(resolvedDisplayName);

    setUserEmail(email);
    setUserName(resolvedDisplayName);
    setLoggedIn(true);

    Logger::instance().info("Registration successful, user: " + email);
    emit registerSucceeded();
}

void AuthBackend::onCodeRequestSuccess(int expiresInSecs)
{
    Q_UNUSED(expiresInSecs);
    setLoading(false);
    
    // Start 60 second cooldown
    codeCooldown_ = 60;
    emit codeCooldownChanged();
    startCooldownTimer();
    
    Logger::instance().info("Verification code sent");
    emit codeRequestSucceeded();
}

void AuthBackend::onAuthRefreshed(const QString& userId,
                                  const QString& email,
                                  const QString& token,
                                  const QString& displayName,
                                  int expiresInSecs)
{
    Q_UNUSED(expiresInSecs);

    setLoading(false);
    Settings::instance().setAuthToken(token);
    Settings::instance().setUserId(userId);
    Settings::instance().setUserEmail(email);

    QString resolvedDisplayName = displayName.trimmed();
    if (resolvedDisplayName.isEmpty()) {
        resolvedDisplayName = Settings::instance().getDisplayName().trimmed();
    }
    if (resolvedDisplayName.isEmpty()) {
        resolvedDisplayName = email.split("@").first();
    }
    Settings::instance().setDisplayName(resolvedDisplayName);

    setUserEmail(email);
    setUserName(resolvedDisplayName);
    setLoggedIn(true);

    Logger::instance().info("Auto-login token refresh successful for: " + email);
}

void AuthBackend::onAuthExpired(const QString& message)
{
    const bool hadSession = isLoggedIn_ || Settings::instance().hasAuthData();
    logout();
    setErrorMessage("登录已过期，请重新登录");
    if (hadSession) {
        emit sessionExpired(message);
    }
}

void AuthBackend::onAuthError(const QString& error)
{
    setLoading(false);
    if (errorMessage_ == QStringLiteral("登录已过期，请重新登录")) {
        return;
    }
    setErrorMessage(error);
    Logger::instance().error("Auth error: " + error);
    emit authFailed(error);
}

void AuthBackend::onCooldownTick()
{
    if (codeCooldown_ > 0) {
        codeCooldown_--;
        emit codeCooldownChanged();
        
        if (codeCooldown_ == 0) {
            cooldownTimer_->stop();
        }
    }
}

void AuthBackend::setLoading(bool loading)
{
    if (loading_ != loading) {
        loading_ = loading;
        emit loadingChanged();
    }
}

void AuthBackend::setErrorMessage(const QString& message)
{
    if (errorMessage_ != message) {
        errorMessage_ = message;
        emit errorMessageChanged();
    }
}

void AuthBackend::setLoggedIn(bool loggedIn)
{
    if (isLoggedIn_ != loggedIn) {
        isLoggedIn_ = loggedIn;
        emit isLoggedInChanged();
    }
}

void AuthBackend::setUserEmail(const QString& email)
{
    if (userEmail_ != email) {
        userEmail_ = email;
        emit userEmailChanged();
    }
}

void AuthBackend::setUserName(const QString& name)
{
    if (userName_ != name) {
        userName_ = name;
        emit userNameChanged();
    }
}

void AuthBackend::startCooldownTimer()
{
    if (!cooldownTimer_->isActive()) {
        cooldownTimer_->start();
    }
}
