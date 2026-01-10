#include "LoginBackend.h"
#include "../utils/settings.h"
#include "../utils/logger.h"
#include <QDateTime>

LoginBackend::LoginBackend(QObject* parent)
    : QObject(parent),
      networkClient_(new NetworkClient(this))
{
    connect(networkClient_, &NetworkClient::tokenReceived, 
            this, &LoginBackend::onTokenReceived);
    connect(networkClient_, &NetworkClient::error, 
            this, &LoginBackend::onNetworkError);
    
    // Set API URL from settings
    networkClient_->setApiUrl(Settings::instance().getSignalingServerUrl());
    
    // Load saved settings
    loadSettings();
}

void LoginBackend::setUserName(const QString& name)
{
    if (userName_ != name) {
        userName_ = name;
        emit userNameChanged();
    }
}

void LoginBackend::setRoomName(const QString& name)
{
    if (roomName_ != name) {
        roomName_ = name;
        emit roomNameChanged();
    }
}

void LoginBackend::setMicEnabled(bool enabled)
{
    if (micEnabled_ != enabled) {
        micEnabled_ = enabled;
        emit micEnabledChanged();
    }
}

void LoginBackend::setCamEnabled(bool enabled)
{
    if (camEnabled_ != enabled) {
        camEnabled_ = enabled;
        emit camEnabledChanged();
    }
}

void LoginBackend::setScheduledTime(const QString& time)
{
    if (scheduledTime_ != time) {
        scheduledTime_ = time;
        emit scheduledTimeChanged();
    }
}

void LoginBackend::setLoading(bool loading)
{
    if (loading_ != loading) {
        loading_ = loading;
        emit loadingChanged();
    }
}

void LoginBackend::setErrorMessage(const QString& message)
{
    if (errorMessage_ != message) {
        errorMessage_ = message;
        emit errorMessageChanged();
    }
}

void LoginBackend::join()
{
    QString name = userName_.trimmed();
    QString room = roomName_.trimmed();
    
    if (name.isEmpty()) {
        setErrorMessage("Please enter your name");
        return;
    }
    
    if (room.isEmpty()) {
        setErrorMessage("Please enter a room name");
        return;
    }
    
    saveSettings();
    Logger::instance().info(QString("Requesting token for room '%1', user '%2'")
                           .arg(room, name));
    
    setLoading(true);
    setErrorMessage("");
    networkClient_->requestToken(room, name);
}

void LoginBackend::quickJoin()
{
    QString name = userName_.trimmed();
    
    if (name.isEmpty()) {
        setErrorMessage("Please enter your name");
        return;
    }
    
    // Generate random room name
    QString randomRoom = QString("room-%1").arg(QDateTime::currentMSecsSinceEpoch());
    setRoomName(randomRoom);
    
    join();
}

void LoginBackend::createScheduledRoom()
{
    QString name = userName_.trimmed();
    
    if (name.isEmpty()) {
        setErrorMessage("Please enter your name");
        return;
    }
    
    QString note = scheduledTime_.trimmed();
    QString suffix = note.isEmpty() ? QString::number(QDateTime::currentMSecsSinceEpoch())
                                    : note.simplified().replace(" ", "-");
    QString privateRoom = QString("scheduled-%1").arg(suffix);
    setRoomName(privateRoom);
    
    join();
}

void LoginBackend::showSettings()
{
    emit settingsRequested();
}

QString LoginBackend::currentTime() const
{
    return QDateTime::currentDateTime().toString("HH:mm");
}

QString LoginBackend::currentDate() const
{
    return QDateTime::currentDateTime().toString("yyyy年MM月dd日 dddd");
}

void LoginBackend::onTokenReceived(const TokenResponse& response)
{
    setLoading(false);
    
    if (!response.success) {
        setErrorMessage("Failed to get token: " + response.error);
        Logger::instance().error("Token request failed: " + response.error);
        return;
    }
    
    Logger::instance().info("Token received, joining conference");
    emit joinConference(response.url, response.token, response.roomName, 
                        userName_, response.isHost);
}

void LoginBackend::onNetworkError(const QString& error)
{
    setLoading(false);
    setErrorMessage("Network error: " + error);
}

void LoginBackend::saveSettings()
{
    Settings::instance().setLastUserName(userName_);
    Settings::instance().setLastRoomName(roomName_);
}

void LoginBackend::loadSettings()
{
    setUserName(Settings::instance().getLastUserName());
    setRoomName(Settings::instance().getLastRoomName());
}
