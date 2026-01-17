#include "ConferenceBackend.h"
#include "../utils/logger.h"
#include "../utils/settings.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QGuiApplication>
#include <QScreen>

ConferenceBackend::ConferenceBackend(QObject* parent)
    : QObject(parent)
    , conferenceManager_(new ConferenceManager(this))
    , shareModeManager_(new ShareModeManager(this))
    , isHost_(false)
{
}

ConferenceBackend::~ConferenceBackend()
{
    if (conferenceManager_ && conferenceManager_->isConnected()) {
        conferenceManager_->disconnect();
    }
}

void ConferenceBackend::initialize(const QString& url, const QString& token,
                                   const QString& roomName, const QString& userName, bool isHost)
{
    url_ = url;
    token_ = token;
    roomName_ = roomName;
    userName_ = userName;
    isHost_ = isHost;
    
    Logger::instance().info(QString("ConferenceBackend initialized for room: %1, isHost: %2")
                           .arg(roomName).arg(isHost));
    
    setupConnections();
    conferenceManager_->connect(url, token);
    
    emit roomNameChanged();
    emit userNameChanged();
}

void ConferenceBackend::setupConnections()
{
    connect(conferenceManager_, &ConferenceManager::connected,
            this, &ConferenceBackend::onConnected);
    connect(conferenceManager_, &ConferenceManager::disconnected,
            this, &ConferenceBackend::onDisconnected);
    connect(conferenceManager_, &ConferenceManager::connectionStateChanged,
            this, &ConferenceBackend::onConnectionStateChanged);
    connect(conferenceManager_, &ConferenceManager::participantJoined,
            this, &ConferenceBackend::onParticipantJoined);
    connect(conferenceManager_, &ConferenceManager::participantLeft,
            this, &ConferenceBackend::onParticipantLeft);
    connect(conferenceManager_, &ConferenceManager::chatMessageReceived,
            this, &ConferenceBackend::onChatMessageReceived);
    connect(conferenceManager_, &ConferenceManager::videoFrameReceived,
            this, &ConferenceBackend::onVideoFrameReceived);
    connect(conferenceManager_, &ConferenceManager::localVideoFrameReady,
            this, &ConferenceBackend::onLocalVideoFrameReady);
    connect(conferenceManager_, &ConferenceManager::localScreenFrameReady,
            this, &ConferenceBackend::onLocalScreenFrameReady);
    connect(conferenceManager_, &ConferenceManager::trackMutedStateChanged,
            this, &ConferenceBackend::onTrackMutedStateChanged);
    connect(conferenceManager_, &ConferenceManager::trackUnsubscribed,
            this, &ConferenceBackend::onTrackUnsubscribed);
    connect(conferenceManager_, &ConferenceManager::trackSubscribed,
            this, &ConferenceBackend::onTrackSubscribed);
    connect(conferenceManager_, &ConferenceManager::trackUnpublished,
            this, &ConferenceBackend::onTrackUnpublished);
    connect(conferenceManager_, &ConferenceManager::localScreenShareChanged,
            this, [this](bool enabled) {
                emit screenSharingChanged();
                // Automatically enter/exit share mode when screen sharing changes
                if (enabled) {
                    shareModeManager_->enterShareMode();
                } else {
                    shareModeManager_->exitShareMode();
                    // Note: localScreenShareEnded is emitted from stopScreenShare() method
                }
            });
    connect(conferenceManager_, &ConferenceManager::localMicrophoneChanged,
            this, [this](bool enabled) {
                if (micState_.value("local", false) == enabled) {
                    return;
                }
                micState_["local"] = enabled;
                emit micEnabledChanged();
                updateParticipantsList();
            });
    connect(conferenceManager_, &ConferenceManager::localCameraChanged,
            this, [this](bool enabled) {
                if (camState_.value("local", false) == enabled) {
                    return;
                }
                camState_["local"] = enabled;
                emit camEnabledChanged();
                updateParticipantsList();
                // Note: localCameraEnded is emitted from toggleCamera() method
            });
}

// Property getters
int ConferenceBackend::participantCount() const
{
    return conferenceManager_ ? conferenceManager_->getParticipantCount() : 1;
}

bool ConferenceBackend::isConnected() const
{
    return conferenceManager_ && conferenceManager_->isConnected();
}

bool ConferenceBackend::micEnabled() const
{
    return conferenceManager_ && conferenceManager_->isMicrophoneEnabled();
}

bool ConferenceBackend::camEnabled() const
{
    return conferenceManager_ && conferenceManager_->isCameraEnabled();
}

bool ConferenceBackend::screenSharing() const
{
    return conferenceManager_ && conferenceManager_->isScreenSharing();
}

// Property setters
void ConferenceBackend::setIsChatVisible(bool visible)
{
    if (isChatVisible_ != visible) {
        isChatVisible_ = visible;
        if (visible) {
            isParticipantsVisible_ = false;
            emit participantsVisibleChanged();
        }
        emit chatVisibleChanged();
    }
}

void ConferenceBackend::setIsParticipantsVisible(bool visible)
{
    if (isParticipantsVisible_ != visible) {
        isParticipantsVisible_ = visible;
        if (visible) {
            isChatVisible_ = false;
            emit chatVisibleChanged();
        }
        emit participantsVisibleChanged();
    }
}

void ConferenceBackend::setSidebarVisible(bool visible)
{
    if (sidebarVisible_ != visible) {
        sidebarVisible_ = visible;
        emit sidebarVisibleChanged();
    }
}

void ConferenceBackend::setViewMode(const QString& mode)
{
    if (viewMode_ != mode) {
        viewMode_ = mode;
        emit viewModeChanged();
    }
}

void ConferenceBackend::setIsFullscreen(bool fullscreen)
{
    if (isFullscreen_ != fullscreen) {
        isFullscreen_ = fullscreen;
        emit fullscreenChanged();
    }
}

void ConferenceBackend::setAlwaysOnTop(bool onTop)
{
    if (alwaysOnTop_ != onTop) {
        alwaysOnTop_ = onTop;
        emit alwaysOnTopChanged();
    }
}

void ConferenceBackend::toggleMainViewSource()
{
    // Toggle between showing camera or screen share in main view
    showScreenShareInMain_ = !showScreenShareInMain_;
    emit showScreenShareInMainChanged();
    
    // Also ensure local video is shown in main view
    if (mainParticipantId_ != "local") {
        mainParticipantId_ = "local";
        emit mainParticipantChanged();
    }
}

void ConferenceBackend::toggleRemoteMainViewSource(const QString& participantId)
{
    // Toggle between showing camera or screen share in main view for this remote participant
    bool current = remoteShowScreenShareInMain_.value(participantId, true);
    remoteShowScreenShareInMain_[participantId] = !current;
    
    // Ensure this participant is shown in main view
    if (mainParticipantId_ != participantId) {
        mainParticipantId_ = participantId;
        emit mainParticipantChanged();
    }
    
    // Notify UI of view state change for this participant
    emit remoteViewStateChanged(participantId);
    
    // Notify UI of participant state change
    updateParticipantsList();
}

bool ConferenceBackend::getRemoteShowScreenInMain(const QString& participantId) const
{
    return remoteShowScreenShareInMain_.value(participantId, true);
}

bool ConferenceBackend::getRemoteScreenSharing(const QString& participantId) const
{
    return screenShareState_.value(participantId, false);
}

// Media controls
void ConferenceBackend::toggleMicrophone()
{
    if (conferenceManager_) {
        conferenceManager_->toggleMicrophone();
        micState_["local"] = conferenceManager_->isMicrophoneEnabled();
        emit micEnabledChanged();
        updateParticipantsList();
    }
}

void ConferenceBackend::toggleCamera()
{
    if (conferenceManager_) {
        bool wasEnabled = conferenceManager_->isCameraEnabled();
        conferenceManager_->toggleCamera();
        camState_["local"] = conferenceManager_->isCameraEnabled();
        emit camEnabledChanged();
        updateParticipantsList();
        // Emit localCameraEnded when camera is turned off
        if (wasEnabled && !conferenceManager_->isCameraEnabled()) {
            emit localCameraEnded();
        }
    }
}

void ConferenceBackend::toggleScreenShare()
{
    if (conferenceManager_) {
        // If already sharing, stop sharing
        if (conferenceManager_->isScreenSharing()) {
            stopScreenShare();
        }
        // Otherwise, the UI will show the picker dialog
    }
}

void ConferenceBackend::startScreenShare(int screenIndex)
{
    if (!conferenceManager_) return;
    
    const auto screens = QGuiApplication::screens();
    if (screenIndex >= 0 && screenIndex < screens.size()) {
        QScreen* screen = screens[screenIndex];
        conferenceManager_->setScreenShareMode(ScreenCapturer::Mode::Screen, screen, 0);
        if (!conferenceManager_->isScreenSharing()) {
            conferenceManager_->toggleScreenShare();
        }
        emit screenSharingChanged();
    }
}

void ConferenceBackend::startWindowShare(qulonglong windowId)
{
    if (!conferenceManager_) return;
    
    WId id = static_cast<WId>(windowId);
    conferenceManager_->setScreenShareMode(ScreenCapturer::Mode::Window, nullptr, id);
    if (!conferenceManager_->isScreenSharing()) {
        conferenceManager_->toggleScreenShare();
    }
    emit screenSharingChanged();
}

void ConferenceBackend::stopScreenShare()
{
    if (conferenceManager_ && conferenceManager_->isScreenSharing()) {
        conferenceManager_->toggleScreenShare();
        emit screenSharingChanged();
        emit localScreenShareEnded();  // Notify QML to clear the frame
    }
}

void ConferenceBackend::switchMicrophone(const QString& deviceId)
{
    if (conferenceManager_) {
        conferenceManager_->switchMicrophone(deviceId);
        updateParticipantsList();
    }
}

void ConferenceBackend::switchCamera(const QString& deviceId)
{
    if (conferenceManager_) {
        conferenceManager_->switchCamera(deviceId);
        updateParticipantsList();
    }
}

// UI controls
void ConferenceBackend::toggleChat()
{
    setIsChatVisible(!isChatVisible_);
}

void ConferenceBackend::toggleParticipants()
{
    setIsParticipantsVisible(!isParticipantsVisible_);
}

void ConferenceBackend::leave()
{
    emit leaveRequested();
}

void ConferenceBackend::confirmLeave()
{
    if (conferenceManager_) {
        conferenceManager_->disconnect();
    }
}

// Chat
void ConferenceBackend::sendChatMessage(const QString& message)
{
    if (conferenceManager_ && !message.trimmed().isEmpty()) {
        conferenceManager_->sendChatMessage(message);
        // Note: Message will be added to the list when received back from server
        // via onChatMessageReceived to avoid duplicates
    }
}

// Participant management
void ConferenceBackend::pinParticipant(const QString& identity)
{
    mainParticipantId_ = identity;
    pinnedMain_ = true;
    emit mainParticipantChanged();
}

void ConferenceBackend::unpinMain()
{
    pinnedMain_ = false;
    // Don't clear mainParticipantId_ to avoid video switching
    // The current participant will remain displayed until user selects another
    emit mainParticipantChanged();
}

void ConferenceBackend::kickParticipant(const QString& identity)
{
    if (!isHost_) {
        Logger::instance().warning("Only hosts can kick participants");
        return;
    }
    
    auto* networkClient = new NetworkClient(this);
    QString apiUrl = Settings::instance().getSignalingServerUrl();
    networkClient->setApiUrl(apiUrl);
    
    Logger::instance().info(QString("Calling kick API for participant: %1").arg(identity));
    networkClient->kickParticipant(roomName_, identity);
    
    QTimer::singleShot(5000, networkClient, &QObject::deleteLater);
}

void ConferenceBackend::muteParticipant(const QString& identity)
{
    bool currentlyMuted = mutedParticipants_.value(identity, false);
    mutedParticipants_[identity] = !currentlyMuted;
    Logger::instance().info(QString("Local mute toggled for %1: %2")
                           .arg(identity).arg(!currentlyMuted ? "muted" : "unmuted"));
}

void ConferenceBackend::hideParticipantVideo(const QString& identity)
{
    bool currentlyHidden = hiddenVideoParticipants_.value(identity, false);
    hiddenVideoParticipants_[identity] = !currentlyHidden;
    Logger::instance().info(QString("Local video visibility toggled for %1")
                           .arg(identity));
}

QVariantMap ConferenceBackend::getParticipantInfo(const QString& identity) const
{
    QVariantMap info;
    info["identity"] = identity;
    info["name"] = nameMap_.value(identity, identity);
    info["micEnabled"] = micState_.value(identity, false);
    info["camEnabled"] = camState_.value(identity, false);
    info["isLocal"] = (identity == "local");
    return info;
}

bool ConferenceBackend::isParticipantMicEnabled(const QString& identity) const
{
    return micState_.value(identity, false);
}

bool ConferenceBackend::isParticipantCamEnabled(const QString& identity) const
{
    return camState_.value(identity, false);
}

// Slots
void ConferenceBackend::onConnected()
{
    Logger::instance().info("Connected to conference");
    connectionStatus_ = "Connected";
    connectionColor_ = "#4caf50";
    emit connectionStatusChanged();
    emit participantCountChanged();
    
    // Initialize local participant state
    micState_["local"] = conferenceManager_->isMicrophoneEnabled();
    camState_["local"] = conferenceManager_->isCameraEnabled();
    nameMap_["local"] = userName_;
    updateParticipantsList();
}

void ConferenceBackend::onDisconnected()
{
    Logger::instance().info("Disconnected from conference");
    connectionStatus_ = "Disconnected";
    connectionColor_ = "#ff5252";
    emit connectionStatusChanged();
}

void ConferenceBackend::onConnectionStateChanged(livekit::ConnectionState state)
{
    switch (state) {
        case livekit::ConnectionState::Connected:
            connectionStatus_ = "Connected";
            connectionColor_ = "#4caf50";
            break;
        case livekit::ConnectionState::Disconnected:
            connectionStatus_ = "Disconnected";
            connectionColor_ = "#ff5252";
            break;
        case livekit::ConnectionState::Reconnecting:
            connectionStatus_ = "Reconnecting...";
            connectionColor_ = "#ff9800";
            break;
        default:
            connectionStatus_ = "Unknown";
            connectionColor_ = "#a0a0b0";
    }
    emit connectionStatusChanged();
}

void ConferenceBackend::onParticipantJoined(const ParticipantInfo& info)
{
    Logger::instance().info(QString("Participant joined: %1").arg(info.name));
    
    nameMap_[info.identity] = info.name.isEmpty() ? info.identity : info.name;
    micState_[info.identity] = info.isMicrophoneEnabled;
    camState_[info.identity] = info.isCameraEnabled;
    
    updateParticipantsList();
    emit participantCountChanged();
    emit participantJoined(info.identity, nameMap_[info.identity]);
}

void ConferenceBackend::onParticipantLeft(const QString& identity)
{
    Logger::instance().info(QString("Participant left: %1").arg(identity));
    
    nameMap_.remove(identity);
    micState_.remove(identity);
    camState_.remove(identity);
    screenShareState_.remove(identity);
    remoteShowScreenShareInMain_.remove(identity);
    
    if (mainParticipantId_ == identity) {
        mainParticipantId_.clear();
        pinnedMain_ = false;
        emit mainParticipantChanged();
    }
    
    updateParticipantsList();
    emit participantCountChanged();
    emit participantLeft(identity);
}

void ConferenceBackend::onChatMessageReceived(const ChatMessage& message)
{
    addChatMessage(message);
}

void ConferenceBackend::onVideoFrameReceived(const QString& participantIdentity,
                                              const QString& trackSid,
                                              const QImage& frame,
                                              livekit::TrackSource source)
{
    // Detect if this is a screen share track
    bool isScreenShare = (source == livekit::TrackSource::SOURCE_SCREENSHARE || 
                          source == livekit::TrackSource::SOURCE_SCREENSHARE_AUDIO);
    
    // Track this track's info for proper cleanup on unsubscribe
    trackInfoMap_[trackSid] = qMakePair(participantIdentity, isScreenShare);
    
    // Track screen share state for this participant
    if (isScreenShare && !screenShareState_.value(participantIdentity, false)) {
        screenShareState_[participantIdentity] = true;
        // Default: show screen in main when first detected
        if (!remoteShowScreenShareInMain_.contains(participantIdentity)) {
            remoteShowScreenShareInMain_[participantIdentity] = true;
        }
        updateParticipantsList();
    } else if (!isScreenShare && !camState_.value(participantIdentity, false)) {
        // Track camera state when first video frame received
        camState_[participantIdentity] = true;
        updateParticipantsList();
    }
    
    // Update main if this is the pinned participant
    if (pinnedMain_ && mainParticipantId_ == participantIdentity) {
        // Frame will be displayed in main area
    } else if (mainParticipantId_.isEmpty()) {
        // Only auto-assign when completely empty (first time)
        mainParticipantId_ = participantIdentity;
        emit mainParticipantChanged();
    }
    // When not pinned and mainParticipantId_ is already set, don't switch
    // User must click to change the main participant
    
    // Emit appropriate signal based on track type
    if (isScreenShare) {
        emit remoteScreenFrameReady(participantIdentity, frame);
    } else {
        emit remoteVideoFrameReady(participantIdentity, frame);
    }
}

void ConferenceBackend::onLocalVideoFrameReady(const QImage& frame)
{
    emit localVideoFrameReady(frame);
}

void ConferenceBackend::onLocalScreenFrameReady(const QImage& frame)
{
    emit localScreenFrameReady(frame);
}

void ConferenceBackend::onTrackSubscribed(const TrackInfo& track)
{
    // Skip local tracks and audio tracks
    if (track.isLocal || track.kind != livekit::TrackKind::KIND_VIDEO) {
        return;
    }
    
    // Determine if this is a screen share track
    bool isScreenShare = (track.source == livekit::TrackSource::SOURCE_SCREENSHARE ||
                          track.source == livekit::TrackSource::SOURCE_SCREENSHARE_AUDIO);
    
    // Record this track's info for proper state handling when mute events fire
    trackInfoMap_[track.trackSid] = qMakePair(track.participantIdentity, isScreenShare);
    
    Logger::instance().info(QString("Track subscribed recorded: %1 from %2 (isScreenShare: %3)")
        .arg(track.trackSid, track.participantIdentity)
        .arg(isScreenShare ? "true" : "false"));
}

void ConferenceBackend::onTrackMutedStateChanged(const QString& trackSid, const QString& id,
                                                  livekit::TrackKind kind, bool muted)
{
    if (kind == livekit::TrackKind::KIND_AUDIO) {
        micState_[id] = !muted;
    } else if (kind == livekit::TrackKind::KIND_VIDEO) {
        // Use trackInfoMap_ to determine if this is camera or screen share
        if (trackInfoMap_.contains(trackSid)) {
            QPair<QString, bool> info = trackInfoMap_[trackSid];
            QString identity = info.first;
            bool isScreenShare = info.second;
            
            if (isScreenShare) {
                screenShareState_[identity] = !muted;
                Logger::instance().info(QString("Screen share %1 for: %2").arg(muted ? "muted" : "unmuted", identity));
                if (muted) {
                    emit remoteTrackEnded(identity, true);  // Clear screen share frame
                }
            } else {
                camState_[identity] = !muted;
                Logger::instance().info(QString("Camera %1 for: %2").arg(muted ? "muted" : "unmuted", identity));
                if (muted) {
                    emit remoteTrackEnded(identity, false);  // Clear camera frame
                }
            }
        } else {
            // Fallback: assume it's camera (old behavior)
            camState_[id] = !muted;
            Logger::instance().info(QString("Video (unknown type) %1 for: %2").arg(muted ? "muted" : "unmuted", id));
            if (muted) {
                emit remoteTrackEnded(id, false);
            }
        }
    }
    
    updateParticipantsList();
}

void ConferenceBackend::onTrackUnsubscribed(const QString& trackSid, const QString& participantIdentity)
{
    // Look up the track info to determine if it's camera or screen share
    if (trackInfoMap_.contains(trackSid)) {
        QPair<QString, bool> info = trackInfoMap_.take(trackSid);
        QString identity = info.first;
        bool isScreenShare = info.second;
        
        if (isScreenShare) {
            screenShareState_[identity] = false;
            emit remoteTrackEnded(identity, true);  // isScreenShare = true
            Logger::instance().info(QString("Screen share ended for: %1").arg(identity));
        } else {
            camState_[identity] = false;
            emit remoteTrackEnded(identity, false);  // isScreenShare = false
            Logger::instance().info(QString("Camera ended for: %1").arg(identity));
        }
        updateParticipantsList();
    } else {
        // Fallback: try to figure out from current state
        Logger::instance().warning(QString("Track unsubscribed without info: %1 from %2").arg(trackSid, participantIdentity));
        
        // Check if this participant had screen share - if so, clear it
        if (screenShareState_.value(participantIdentity, false)) {
            screenShareState_[participantIdentity] = false;
            emit remoteTrackEnded(participantIdentity, true);
            updateParticipantsList();
        }
        // Also check camera
        if (camState_.value(participantIdentity, false)) {
            camState_[participantIdentity] = false;
            emit remoteTrackEnded(participantIdentity, false);
            updateParticipantsList();
        }
    }
}

void ConferenceBackend::onTrackUnpublished(const QString& trackSid, const QString& participantIdentity,
                                            livekit::TrackKind kind, livekit::TrackSource source)
{
    Q_UNUSED(trackSid);
    
    // Only handle video tracks
    if (kind != livekit::TrackKind::KIND_VIDEO) {
        return;
    }
    
    // Determine if this is a screen share track based on source
    bool isScreenShare = (source == livekit::TrackSource::SOURCE_SCREENSHARE ||
                          source == livekit::TrackSource::SOURCE_SCREENSHARE_AUDIO);
    
    if (isScreenShare) {
        Logger::instance().info(QString("Screen share unpublished for: %1").arg(participantIdentity));
        screenShareState_[participantIdentity] = false;
        emit remoteTrackEnded(participantIdentity, true);  // isScreenShare = true
    } else {
        Logger::instance().info(QString("Camera unpublished for: %1").arg(participantIdentity));
        camState_[participantIdentity] = false;
        emit remoteTrackEnded(participantIdentity, false);  // isScreenShare = false
    }
    
    // Clean up trackInfoMap_
    if (trackInfoMap_.contains(trackSid)) {
        trackInfoMap_.remove(trackSid);
    }
    
    updateParticipantsList();
}

void ConferenceBackend::updateParticipantsList()
{
    participants_.clear();
    
    // Add local participant first
    QVariantMap localParticipant;
    localParticipant["identity"] = "local";
    localParticipant["name"] = userName_;
    localParticipant["micEnabled"] = micEnabled();
    localParticipant["camEnabled"] = camEnabled();
    localParticipant["screenSharing"] = screenSharing();  // Local screen share state
    localParticipant["isLocal"] = true;
    localParticipant["isHost"] = isHost_;
    participants_.append(localParticipant);
    
    // Add remote participants
    for (auto it = nameMap_.cbegin(); it != nameMap_.cend(); ++it) {
        if (it.key() != "local") {
            QVariantMap participant;
            participant["identity"] = it.key();
            participant["name"] = it.value();
            participant["micEnabled"] = micState_.value(it.key(), false);
            participant["camEnabled"] = camState_.value(it.key(), false);
            participant["screenSharing"] = screenShareState_.value(it.key(), false);  // Remote screen share state
            participant["isLocal"] = false;
            participant["isHost"] = false;
            participants_.append(participant);
        }
    }
    
    emit participantsChanged();
}

void ConferenceBackend::addChatMessage(const ChatMessage& msg)
{
    QVariantMap message;
    message["sender"] = msg.sender;
    message["senderIdentity"] = msg.senderIdentity;
    message["message"] = msg.message;
    message["timestamp"] = msg.timestamp;
    message["isLocal"] = msg.isLocal;
    
    chatMessages_.append(message);
    emit chatMessagesChanged();
}
