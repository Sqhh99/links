#include "network_client.h"
#include "../utils/logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>

NetworkClient::NetworkClient(QObject* parent)
    : QObject(parent),
      networkManager_(new QNetworkAccessManager(this)),
      apiUrl_("https://sqhh99.dpdns.org:8443")
{
}

NetworkClient::~NetworkClient()
{
}

void NetworkClient::setApiUrl(const QString& url)
{
    // Remove trailing slash to prevent double slashes in URL construction
    apiUrl_ = url;
    if (apiUrl_.endsWith('/')) {
        apiUrl_.chop(1);
    }
    Logger::instance().info("API URL set to: " + apiUrl_);
}

void NetworkClient::requestToken(const QString& roomName, const QString& participantName)
{
    Logger::instance().info(QString("Requesting token for room '%1', participant '%2'")
                           .arg(roomName, participantName));
    
    QUrl url(apiUrl_ + "/api/token");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject json;
    json["roomName"] = roomName;
    json["participantName"] = participantName;
    
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();
    
    QNetworkReply* reply = networkManager_->post(request, data);
    connect(reply, &QNetworkReply::finished, this, &NetworkClient::onTokenReplyFinished);
}

void NetworkClient::createRoom(const QString& roomName)
{
    Logger::instance().info("Creating room: " + roomName);
    
    QUrl url(apiUrl_ + "/api/rooms");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject json;
    json["name"] = roomName;
    
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();
    
    QNetworkReply* reply = networkManager_->post(request, data);
    connect(reply, &QNetworkReply::finished, this, &NetworkClient::onCreateRoomReplyFinished);
}

void NetworkClient::listRooms()
{
    Logger::instance().info("Listing rooms");
    
    QUrl url(apiUrl_ + "/api/rooms");
    QNetworkRequest request(url);
    
    QNetworkReply* reply = networkManager_->get(request);
    connect(reply, &QNetworkReply::finished, this, &NetworkClient::onListRoomsReplyFinished);
}

void NetworkClient::onTokenReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    reply->deleteLater();
    
    TokenResponse response;
    
    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        response.success = false;
        response.error = reply->errorString();
        emit tokenReceived(response);
        return;
    }
    
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        response.token = obj["token"].toString();
        response.url = obj["url"].toString();
        response.roomName = obj["roomName"].toString();
        response.isHost = obj["isHost"].toBool(false);
        response.success = true;
        
        Logger::instance().info(QString("Token received successfully (isHost: %1)").arg(response.isHost));
        emit tokenReceived(response);
    } else {
        response.success = false;
        response.error = "Invalid response format";
        Logger::instance().error("Invalid token response format");
        emit tokenReceived(response);
    }
}

void NetworkClient::onCreateRoomReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    reply->deleteLater();
    
    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        return;
    }
    
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        QString roomName = obj["name"].toString();
        Logger::instance().info("Room created: " + roomName);
        emit roomCreated(roomName);
    }
}

void NetworkClient::onListRoomsReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    reply->deleteLater();
    
    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        return;
    }
    
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isArray()) {
        QJsonArray rooms = doc.array();
        Logger::instance().info(QString("Received %1 rooms").arg(rooms.size()));
        emit roomsListed(rooms);
    }
}

void NetworkClient::handleNetworkError(QNetworkReply* reply)
{
    QString errorMsg = QString("Network error: %1").arg(reply->errorString());
    Logger::instance().error(errorMsg);
    emit error(errorMsg);
}

void NetworkClient::kickParticipant(const QString& roomName, const QString& identity)
{
    Logger::instance().info(QString("Kicking participant '%1' from room '%2'").arg(identity, roomName));
    
    QUrl url(apiUrl_ + QString("/api/rooms/%1/participants/%2").arg(roomName, identity));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = networkManager_->deleteResource(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, identity]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            QString errorMsg = QString("Failed to kick participant: %1").arg(reply->errorString());
            Logger::instance().error(errorMsg);
            emit error(errorMsg);
            return;
        }
        
        Logger::instance().info(QString("Successfully kicked participant '%1'").arg(identity));
    });
}

void NetworkClient::endRoom(const QString& roomName)
{
    Logger::instance().info(QString("Ending room '%1'").arg(roomName));
    
    QUrl url(apiUrl_ + QString("/api/rooms/%1/end").arg(roomName));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = networkManager_->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply, roomName]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            QString errorMsg = QString("Failed to end room: %1").arg(reply->errorString());
            Logger::instance().error(errorMsg);
            emit error(errorMsg);
            return;
        }
        
        Logger::instance().info(QString("Successfully ended room '%1'").arg(roomName));
    });
}
