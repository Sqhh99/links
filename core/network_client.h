#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QJsonObject>

struct TokenResponse {
    QString token;
    QString url;
    QString roomName;
    bool success;
    bool isHost{false};
    QString error;
};

class NetworkClient : public QObject
{
    Q_OBJECT
    
public:
    explicit NetworkClient(QObject* parent = nullptr);
    ~NetworkClient() override;
    
    void requestToken(const QString& roomName, const QString& participantName);
    void createRoom(const QString& roomName);
    void listRooms();
    void kickParticipant(const QString& roomName, const QString& identity);
    void endRoom(const QString& roomName);
    
    // Auth APIs
    void login(const QString& email, const QString& password);
    void requestVerificationCode(const QString& email);
    void registerUser(const QString& email, const QString& password, const QString& code);
    
    void setApiUrl(const QString& url);
    QString getApiUrl() const { return apiUrl_; }
    
signals:
    void tokenReceived(const TokenResponse& response);
    void roomCreated(const QString& roomName);
    void roomsListed(const QJsonArray& rooms);
    void error(const QString& message);
    
    // Auth signals
    void loginSuccess(const QString& userId, const QString& email, const QString& token);
    void registerSuccess(const QString& userId, const QString& email, const QString& token);
    void codeRequestSuccess(int expiresInSecs);
    void authError(const QString& message);
    
private slots:
    void onTokenReplyFinished();
    void onCreateRoomReplyFinished();
    void onListRoomsReplyFinished();
    
private:
    void handleNetworkError(QNetworkReply* reply);
    
    QNetworkAccessManager* networkManager_;
    QString apiUrl_;
};

#endif // NETWORK_CLIENT_H
