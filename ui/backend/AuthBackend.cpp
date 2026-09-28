#include "AuthBackend.h"
#include "../utils/settings.h"
#include "../utils/logger.h"

namespace {

// Accounts created by the old email sign-up use the email as their username;
// show only the part before '@' rather than the whole address.
QString fallbackDisplayName(const QString& username)
{
    return username.section('@', 0, 0);
}

} // namespace

AuthBackend::AuthBackend(QObject* parent)
    : QObject(parent),
      networkClient_(new NetworkClient(this))
{
    // Connect network signals
    connect(networkClient_, &NetworkClient::loginSuccess,
            this, &AuthBackend::onLoginSuccess);
    connect(networkClient_, &NetworkClient::authRefreshed,
            this, &AuthBackend::onAuthRefreshed);
    connect(networkClient_, &NetworkClient::authExpired,
            this, &AuthBackend::onAuthExpired);
    connect(networkClient_, &NetworkClient::authError,
            this, &AuthBackend::onAuthError);
    
    // Set API URL from settings
    networkClient_->setApiUrl(Settings::instance().getSignalingServerUrl());
    
    // Try auto-login on creation
    tryAutoLogin();
}

void AuthBackend::login(const QString& username, const QString& password)
{
    if (loading_) return;
    
    setLoading(true);
    setErrorMessage("");
    networkClient_->login(username.trimmed(), password);
}

void AuthBackend::logout()
{
    Settings::instance().clearAuthData();
    setAuthToken("");
    setLoggedIn(false);
    setAccountName("");
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

void AuthBackend::onLoginSuccess(const QString& userId, const QString& username, const QString& token,
                                 const QString& displayName)
{
    setLoading(false);
    
    // Save auth data
    Settings::instance().setAuthToken(token);
    Settings::instance().setUserId(userId);
    Settings::instance().setUsername(username);
    setAuthToken(token.trimmed());
    
    QString resolvedDisplayName = displayName.trimmed();
    if (resolvedDisplayName.isEmpty()) {
        resolvedDisplayName = fallbackDisplayName(username);
    }
    Settings::instance().setDisplayName(resolvedDisplayName);

    setAccountName(username);
    setUserName(resolvedDisplayName);
    setLoggedIn(true);
    
    Logger::instance().info("Login successful, user: " + username);
    emit loginSucceeded();
}

void AuthBackend::onAuthRefreshed(const QString& userId,
                                  const QString& username,
                                  const QString& token,
                                  const QString& displayName,
                                  int expiresInSecs)
{
    Q_UNUSED(expiresInSecs);

    setLoading(false);
    Settings::instance().setAuthToken(token);
    Settings::instance().setUserId(userId);
    Settings::instance().setUsername(username);
    setAuthToken(token.trimmed());

    QString resolvedDisplayName = displayName.trimmed();
    if (resolvedDisplayName.isEmpty()) {
        resolvedDisplayName = Settings::instance().getDisplayName().trimmed();
    }
    if (resolvedDisplayName.isEmpty()) {
        resolvedDisplayName = fallbackDisplayName(username);
    }
    Settings::instance().setDisplayName(resolvedDisplayName);

    setAccountName(username);
    setUserName(resolvedDisplayName);
    setLoggedIn(true);

    Logger::instance().info("Auto-login token refresh successful for: " + username);
}

void AuthBackend::onAuthExpired(const QString& message)
{
    const bool hadSession = isLoggedIn_ || !authToken_.isEmpty();
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

void AuthBackend::setAccountName(const QString& username)
{
    if (accountName_ != username) {
        accountName_ = username;
        emit accountNameChanged();
    }
}

void AuthBackend::setUserName(const QString& name)
{
    if (userName_ != name) {
        userName_ = name;
        emit userNameChanged();
    }
}

void AuthBackend::setAuthToken(const QString& token)
{
    const QString trimmed = token.trimmed();
    if (authToken_ == trimmed) {
        return;
    }

    authToken_ = trimmed;
    emit authTokenChanged();
}
