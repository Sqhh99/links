#ifndef LOGINBACKEND_H
#define LOGINBACKEND_H

#include <QObject>
#include <QString>
#include "../core/network_client.h"

class LoginBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString userName READ userName WRITE setUserName NOTIFY userNameChanged)
    Q_PROPERTY(QString roomName READ roomName WRITE setRoomName NOTIFY roomNameChanged)
    Q_PROPERTY(bool micEnabled READ micEnabled WRITE setMicEnabled NOTIFY micEnabledChanged)
    Q_PROPERTY(bool camEnabled READ camEnabled WRITE setCamEnabled NOTIFY camEnabledChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString scheduledTime READ scheduledTime WRITE setScheduledTime NOTIFY scheduledTimeChanged)

public:
    explicit LoginBackend(QObject* parent = nullptr);
    ~LoginBackend() override = default;

    QString userName() const { return userName_; }
    void setUserName(const QString& name);
    
    QString roomName() const { return roomName_; }
    void setRoomName(const QString& name);
    
    bool micEnabled() const { return micEnabled_; }
    void setMicEnabled(bool enabled);
    
    bool camEnabled() const { return camEnabled_; }
    void setCamEnabled(bool enabled);
    
    bool loading() const { return loading_; }
    QString errorMessage() const { return errorMessage_; }
    
    QString scheduledTime() const { return scheduledTime_; }
    void setScheduledTime(const QString& time);

    Q_INVOKABLE void join();
    Q_INVOKABLE void quickJoin();
    Q_INVOKABLE void createScheduledRoom();
    Q_INVOKABLE void showSettings();
    Q_INVOKABLE QString currentTime() const;
    Q_INVOKABLE QString currentDate() const;

signals:
    void userNameChanged();
    void roomNameChanged();
    void micEnabledChanged();
    void camEnabledChanged();
    void loadingChanged();
    void errorMessageChanged();
    void scheduledTimeChanged();
    void joinConference(const QString& url, const QString& token, 
                        const QString& roomName, const QString& userName, bool isHost);
    void settingsRequested();

private slots:
    void onTokenReceived(const TokenResponse& response);
    void onNetworkError(const QString& error);

private:
    void setLoading(bool loading);
    void setErrorMessage(const QString& message);
    void saveSettings();
    void loadSettings();

    NetworkClient* networkClient_;
    QString userName_;
    QString roomName_;
    QString scheduledTime_;
    bool micEnabled_{false};
    bool camEnabled_{false};
    bool loading_{false};
    QString errorMessage_;
};

#endif // LOGINBACKEND_H
