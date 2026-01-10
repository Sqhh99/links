/*
 * Copyright 2023 LiveKit
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "livekit/room.h"
#include "livekit/ffi_client.h"
#include "ffi.pb.h"
#include "room.pb.h"
#include <functional>
#include <iostream>
#include <cstring>

namespace livekit
{

using proto::FfiRequest;
using proto::FfiResponse;
using proto::ConnectRequest;
using proto::RoomOptions;
using proto::ConnectCallback;
using proto::FfiEvent;

// ============================================================================
// Constructor / Destructor
// ============================================================================

Room::Room() 
    : handle_(INVALID_HANDLE),
      listener_id_(0),
      connected_(false),
      connectAsyncId_(0),
      connection_state_(proto::CONN_DISCONNECTED)
{
}

Room::~Room()
{
    if (listener_id_ != 0) {
        FfiClient::getInstance().RemoveListener(listener_id_);
    }
}

// ============================================================================
// Connection
// ============================================================================

void Room::Connect(const std::string& url, const std::string& token) 
{
    // Register listener first (outside Room lock to avoid lock inversion)
    auto listenerId = FfiClient::getInstance().AddListener(
        std::bind(&Room::OnEvent, this, std::placeholders::_1));

    // Build request
    proto::FfiRequest req;
    auto* connect = req.mutable_connect();
    connect->set_url(url);
    connect->set_token(token);
    connect->mutable_options()->set_auto_subscribe(true);

    // Mark "connecting" under lock
    {
        std::lock_guard<std::mutex> g(lock_);
        if (connected_) {
            FfiClient::getInstance().RemoveListener(listenerId);
            throw std::runtime_error("already connected");
        }
        listener_id_ = listenerId;
        connectAsyncId_ = listenerId;
    }

    // Call into FFI with no Room lock held (avoid re-entrancy deadlock)
    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    // Store async id under lock
    {
        std::lock_guard<std::mutex> g(lock_);
        connectAsyncId_ = resp.connect().async_id();
    }
}

void Room::Disconnect()
{
    std::unique_lock<std::mutex> lk(lock_);

    if (!connected_ || handle_ == INVALID_HANDLE) {
        return;
    }

    proto::FfiRequest req;
    auto* disconnect = req.mutable_disconnect();
    disconnect->set_room_handle(static_cast<uint64_t>(handle_));

    lk.unlock();
    FfiClient::getInstance().SendRequest(req);
    lk.lock();

    connected_ = false;
    connection_state_ = proto::CONN_DISCONNECTED;
}

// ============================================================================
// Participant Access
// ============================================================================

std::shared_ptr<LocalParticipant> Room::GetLocalParticipant() const
{
    std::lock_guard<std::mutex> g(lock_);
    return local_participant_;
}

std::vector<std::shared_ptr<RemoteParticipant>> Room::GetRemoteParticipants() const
{
    std::lock_guard<std::mutex> g(lock_);
    std::vector<std::shared_ptr<RemoteParticipant>> participants;
    participants.reserve(remote_participants_.size());
    for (const auto& [_, participant] : remote_participants_) {
        participants.push_back(participant);
    }
    return participants;
}

std::shared_ptr<RemoteParticipant> Room::GetParticipant(const std::string& identity) const
{
    std::lock_guard<std::mutex> g(lock_);
    auto it = remote_participants_.find(identity);
    return it != remote_participants_.end() ? it->second : nullptr;
}

uint64_t Room::CreateVideoStream(std::shared_ptr<RemoteTrack> track,
                                 const std::string& participant_identity,
                                 VideoStreamCallback callback,
                                 proto::VideoBufferType format,
                                 bool normalize_stride)
{
    if (!track || !callback) {
        return 0;
    }

    {
        std::lock_guard<std::mutex> g(lock_);
        auto existing = track_video_streams_.find(track->GetSid());
        if (existing != track_video_streams_.end()) {
            return existing->second;
        }
    }

    proto::FfiRequest req;
    auto* new_stream = req.mutable_new_video_stream();
    new_stream->set_track_handle(static_cast<uint64_t>(track->GetHandle()));
    new_stream->set_type(proto::VIDEO_STREAM_NATIVE);
    new_stream->set_format(format);
    new_stream->set_normalize_stride(normalize_stride);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    if (!resp.has_new_video_stream()) {
        throw std::runtime_error("Failed to create video stream");
    }

    const auto& owned_stream = resp.new_video_stream().stream();
    uint64_t handle_id = owned_stream.handle().id();

    std::lock_guard<std::mutex> g(lock_);
    VideoStreamState state{
        FfiHandle(static_cast<uintptr_t>(handle_id)),
        track->GetSid(),
        participant_identity,
        std::move(callback)
    };
    video_streams_.emplace(handle_id, std::move(state));
    track_video_streams_[track->GetSid()] = handle_id;

    return handle_id;
}

uint64_t Room::CreateAudioStream(std::shared_ptr<RemoteTrack> track,
                                 const std::string& participant_identity,
                                 AudioStreamCallback callback,
                                 uint32_t sample_rate,
                                 uint32_t num_channels)
{
    if (!track || !callback) {
        return 0;
    }

    {
        std::lock_guard<std::mutex> g(lock_);
        auto existing = track_audio_streams_.find(track->GetSid());
        if (existing != track_audio_streams_.end()) {
            return existing->second;
        }
    }

    proto::FfiRequest req;
    auto* new_stream = req.mutable_new_audio_stream();
    new_stream->set_track_handle(static_cast<uint64_t>(track->GetHandle()));
    new_stream->set_type(proto::AUDIO_STREAM_NATIVE);
    new_stream->set_sample_rate(sample_rate);
    new_stream->set_num_channels(num_channels);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    if (!resp.has_new_audio_stream()) {
        throw std::runtime_error("Failed to create audio stream");
    }

    const auto& owned_stream = resp.new_audio_stream().stream();
    uint64_t handle_id = owned_stream.handle().id();

    std::lock_guard<std::mutex> g(lock_);
    AudioStreamState state{
        FfiHandle(static_cast<uintptr_t>(handle_id)),
        track->GetSid(),
        participant_identity,
        std::move(callback)
    };
    audio_streams_.emplace(handle_id, std::move(state));
    track_audio_streams_[track->GetSid()] = handle_id;

    return handle_id;
}

void Room::RemoveVideoStream(uint64_t stream_handle)
{
    std::lock_guard<std::mutex> g(lock_);
    auto it = video_streams_.find(stream_handle);
    if (it != video_streams_.end()) {
        track_video_streams_.erase(it->second.track_sid);
        video_streams_.erase(it);
    }
}

void Room::RemoveAudioStream(uint64_t stream_handle)
{
    std::lock_guard<std::mutex> g(lock_);
    auto it = audio_streams_.find(stream_handle);
    if (it != audio_streams_.end()) {
        track_audio_streams_.erase(it->second.track_sid);
        audio_streams_.erase(it);
    }
}

void Room::RemoveStreamsForTrack(const std::string& track_sid)
{
    std::lock_guard<std::mutex> g(lock_);
    RemoveStreamsForTrackLocked(track_sid);
}

void Room::RemoveStreamsForTrackLocked(const std::string& track_sid)
{
    auto v = track_video_streams_.find(track_sid);
    if (v != track_video_streams_.end()) {
        video_streams_.erase(v->second);
        track_video_streams_.erase(v);
    }

    auto a = track_audio_streams_.find(track_sid);
    if (a != track_audio_streams_.end()) {
        audio_streams_.erase(a->second);
        track_audio_streams_.erase(a);
    }
}

// ============================================================================
// Event Callbacks
// ============================================================================

void Room::SetOnParticipantConnected(ParticipantConnectedCallback callback)
{
    std::lock_guard<std::mutex> g(lock_);
    on_participant_connected_ = std::move(callback);
}

void Room::SetOnParticipantDisconnected(ParticipantDisconnectedCallback callback)
{
    std::lock_guard<std::mutex> g(lock_);
    on_participant_disconnected_ = std::move(callback);
}

void Room::SetOnTrackPublished(TrackPublishedCallback callback)
{
    std::lock_guard<std::mutex> g(lock_);
    on_track_published_ = std::move(callback);
}

void Room::SetOnTrackUnpublished(TrackUnpublishedCallback callback)
{
    std::lock_guard<std::mutex> g(lock_);
    on_track_unpublished_ = std::move(callback);
}

void Room::SetOnTrackSubscribed(TrackSubscribedCallback callback)
{
    std::lock_guard<std::mutex> g(lock_);
    on_track_subscribed_ = std::move(callback);
}

void Room::SetOnTrackUnsubscribed(TrackUnsubscribedCallback callback)
{
    std::lock_guard<std::mutex> g(lock_);
    on_track_unsubscribed_ = std::move(callback);
}

void Room::SetOnTrackMuted(TrackMutedCallback callback)
{
    std::lock_guard<std::mutex> g(lock_);
    on_track_muted_ = std::move(callback);
}

void Room::SetOnDataReceived(DataReceivedCallback callback)
{
    std::lock_guard<std::mutex> g(lock_);
    on_data_received_ = std::move(callback);
}

void Room::SetOnConnectionStateChanged(ConnectionStateChangedCallback callback)
{
    std::lock_guard<std::mutex> g(lock_);
    on_connection_state_changed_ = std::move(callback);
}

void Room::SetOnActiveSpeakersChanged(ActiveSpeakersChangedCallback callback)
{
    std::lock_guard<std::mutex> g(lock_);
    on_active_speakers_changed_ = std::move(callback);
}

void Room::SetOnRoomMetadataChanged(RoomMetadataChangedCallback callback)
{
    std::lock_guard<std::mutex> g(lock_);
    on_room_metadata_changed_ = std::move(callback);
}

void Room::SetOnChatMessageReceived(ChatMessageReceivedCallback callback)
{
    std::lock_guard<std::mutex> g(lock_);
    on_chat_message_received_ = std::move(callback);
}

void Room::SetOnTranscriptionReceived(TranscriptionReceivedCallback callback)
{
    std::lock_guard<std::mutex> g(lock_);
    on_transcription_received_ = std::move(callback);
}

// ============================================================================
// Chat Messaging
// ============================================================================

void Room::SendChatMessage(const std::string& message,
                          const std::vector<std::string>& destination_identities)
{
    std::lock_guard<std::mutex> g(lock_);
    if (!connected_ || handle_.handle == INVALID_HANDLE) {
        std::cerr << "Cannot send chat message: not connected" << std::endl;
        return;
    }

    proto::FfiRequest request;
    auto* chat_request = request.mutable_send_chat_message();
    chat_request->set_local_participant_handle(handle_.handle);
    chat_request->set_message(message);
    for (const auto& identity : destination_identities) {
        chat_request->add_destination_identities(identity);
    }

    auto response = FfiClient::getInstance().SendRequest(request);
    if (response.has_send_chat_message()) {
        std::cout << "Chat message sent" << std::endl;
    }
}

void Room::EditChatMessage(const std::string& edit_text,
                          const std::string& original_message_id,
                          int64_t original_timestamp,
                          const std::string& original_message,
                          const std::vector<std::string>& destination_identities)
{
    std::lock_guard<std::mutex> g(lock_);
    if (!connected_ || handle_.handle == INVALID_HANDLE) {
        std::cerr << "Cannot edit chat message: not connected" << std::endl;
        return;
    }

    proto::FfiRequest request;
    auto* edit_request = request.mutable_edit_chat_message();
    edit_request->set_local_participant_handle(handle_.handle);
    edit_request->set_edit_text(edit_text);
    
    auto* original = edit_request->mutable_original_message();
    original->set_id(original_message_id);
    original->set_timestamp(original_timestamp);
    original->set_message(original_message);
    
    for (const auto& identity : destination_identities) {
        edit_request->add_destination_identities(identity);
    }

    FfiClient::getInstance().SendRequest(request);
    // EditChatMessage is async, result will come via SendChatMessageCallback
    std::cout << "Chat message edit request sent" << std::endl;
}

// ============================================================================
// Session Statistics
// ============================================================================

void Room::GetSessionStats(std::function<void(const SessionStats& stats, const std::string& error)> callback)
{
    std::lock_guard<std::mutex> g(lock_);
    if (!connected_ || handle_.handle == INVALID_HANDLE) {
        if (callback) {
            callback(SessionStats{}, "Not connected");
        }
        return;
    }

    proto::FfiRequest request;
    auto* stats_request = request.mutable_get_session_stats();
    stats_request->set_room_handle(handle_.handle);

    auto response = FfiClient::getInstance().SendRequest(request);
    if (response.has_get_session_stats()) {
        uint64_t async_id = response.get_session_stats().async_id();
        session_stats_callbacks_[async_id] = std::move(callback);
    } else if (callback) {
        callback(SessionStats{}, "Failed to request session stats");
    }
}

std::vector<std::string> Room::GetActiveSpeakers() const
{
    std::lock_guard<std::mutex> g(lock_);
    return active_speakers_;
}

void Room::OnGetSessionStatsCallback(const proto::GetSessionStatsCallback& cb)
{
    std::function<void(const SessionStats&, const std::string&)> callback;
    {
        std::lock_guard<std::mutex> g(lock_);
        auto it = session_stats_callbacks_.find(cb.async_id());
        if (it == session_stats_callbacks_.end()) {
            return;
        }
        callback = std::move(it->second);
        session_stats_callbacks_.erase(it);
    }

    if (!callback) {
        return;
    }

    if (cb.message_case() == proto::GetSessionStatsCallback::kError) {
        callback(SessionStats{}, cb.error());
        return;
    }

    SessionStats stats;
    const auto& result = cb.result();
    for (const auto& stat : result.publisher_stats()) {
        stats.publisher_stats.push_back(stat);
    }
    for (const auto& stat : result.subscriber_stats()) {
        stats.subscriber_stats.push_back(stat);
    }

    callback(stats, "");
}

// ============================================================================
// RPC
// ============================================================================

void Room::RegisterRpcMethod(const std::string& method, RpcMethodHandler handler)
{
    {
        std::lock_guard<std::mutex> g(lock_);
        if (!connected_ || !local_participant_) {
            std::cerr << "Cannot register RPC method: not connected" << std::endl;
            return;
        }
        rpc_method_handlers_[method] = std::move(handler);
    }

    proto::FfiRequest request;
    auto* rpc_request = request.mutable_register_rpc_method();
    rpc_request->set_local_participant_handle(local_participant_->GetHandleId());
    rpc_request->set_method(method);

    auto response = FfiClient::getInstance().SendRequest(request);
    std::cout << "RPC method registered: " << method << std::endl;
}

void Room::UnregisterRpcMethod(const std::string& method)
{
    {
        std::lock_guard<std::mutex> g(lock_);
        if (!connected_ || !local_participant_) {
            return;
        }
        rpc_method_handlers_.erase(method);
    }

    proto::FfiRequest request;
    auto* rpc_request = request.mutable_unregister_rpc_method();
    rpc_request->set_local_participant_handle(local_participant_->GetHandleId());
    rpc_request->set_method(method);

    FfiClient::getInstance().SendRequest(request);
    std::cout << "RPC method unregistered: " << method << std::endl;
}

void Room::PerformRpc(const std::string& destination_identity,
                     const std::string& method,
                     const std::string& payload,
                     RpcResponseCallback callback,
                     uint32_t response_timeout_ms)
{
    std::lock_guard<std::mutex> g(lock_);
    if (!connected_ || !local_participant_) {
        if (callback) {
            RpcError err{1, "Not connected", ""};
            callback("", &err);
        }
        return;
    }

    proto::FfiRequest request;
    auto* rpc_request = request.mutable_perform_rpc();
    rpc_request->set_local_participant_handle(local_participant_->GetHandleId());
    rpc_request->set_destination_identity(destination_identity);
    rpc_request->set_method(method);
    rpc_request->set_payload(payload);
    rpc_request->set_response_timeout_ms(response_timeout_ms);

    auto response = FfiClient::getInstance().SendRequest(request);
    if (response.has_perform_rpc()) {
        uint64_t async_id = response.perform_rpc().async_id();
        rpc_response_callbacks_[async_id] = std::move(callback);
    } else if (callback) {
        RpcError err{1, "Failed to perform RPC", ""};
        callback("", &err);
    }
}

void Room::OnPerformRpcCallback(const proto::PerformRpcCallback& cb)
{
    RpcResponseCallback callback;
    {
        std::lock_guard<std::mutex> g(lock_);
        auto it = rpc_response_callbacks_.find(cb.async_id());
        if (it == rpc_response_callbacks_.end()) {
            return;
        }
        callback = std::move(it->second);
        rpc_response_callbacks_.erase(it);
    }

    if (!callback) {
        return;
    }

    if (cb.has_error()) {
        RpcError err;
        err.code = cb.error().code();
        err.message = cb.error().message();
        if (cb.error().has_data()) {
            err.data = cb.error().data();
        }
        callback("", &err);
    } else {
        callback(cb.has_payload() ? cb.payload() : "", nullptr);
    }
}

void Room::OnRpcMethodInvocationEvent(const proto::RpcMethodInvocationEvent& event)
{
    RpcMethodHandler handler;
    {
        std::lock_guard<std::mutex> g(lock_);
        auto it = rpc_method_handlers_.find(event.method());
        if (it == rpc_method_handlers_.end()) {
            // No handler registered, respond with error
            RespondToRpcInvocationWithError(event.invocation_id(), 
                RpcError{1, "Method not found", ""});
            return;
        }
        handler = it->second;
    }

    if (!handler) {
        RespondToRpcInvocationWithError(event.invocation_id(), 
            RpcError{1, "Handler not available", ""});
        return;
    }

    RpcMethodInvocation invocation;
    invocation.invocation_id = event.invocation_id();
    invocation.method = event.method();
    invocation.request_id = event.request_id();
    invocation.caller_identity = event.caller_identity();
    invocation.payload = event.payload();
    invocation.response_timeout_ms = event.response_timeout_ms();

    uint64_t invocation_id = event.invocation_id();
    
    auto respond = [this, invocation_id](const std::string& payload) {
        RespondToRpcInvocation(invocation_id, payload);
    };
    
    auto respond_error = [this, invocation_id](const RpcError& error) {
        RespondToRpcInvocationWithError(invocation_id, error);
    };

    handler(invocation, std::move(respond), std::move(respond_error));
}

void Room::RespondToRpcInvocation(uint64_t invocation_id, const std::string& payload)
{
    std::lock_guard<std::mutex> g(lock_);
    if (!connected_ || !local_participant_) {
        return;
    }

    proto::FfiRequest request;
    auto* rpc_response = request.mutable_rpc_method_invocation_response();
    rpc_response->set_local_participant_handle(local_participant_->GetHandleId());
    rpc_response->set_invocation_id(invocation_id);
    rpc_response->set_payload(payload);

    FfiClient::getInstance().SendRequest(request);
}

void Room::RespondToRpcInvocationWithError(uint64_t invocation_id, const RpcError& error)
{
    std::lock_guard<std::mutex> g(lock_);
    if (!connected_ || !local_participant_) {
        return;
    }

    proto::FfiRequest request;
    auto* rpc_response = request.mutable_rpc_method_invocation_response();
    rpc_response->set_local_participant_handle(local_participant_->GetHandleId());
    rpc_response->set_invocation_id(invocation_id);
    
    auto* err = rpc_response->mutable_error();
    err->set_code(error.code);
    err->set_message(error.message);
    if (!error.data.empty()) {
        err->set_data(error.data);
    }

    FfiClient::getInstance().SendRequest(request);
}

// ============================================================================
// Event Handling
// ============================================================================

void Room::OnEvent(const FfiEvent& event) 
{
    try {
        switch (event.message_case()) {
            case FfiEvent::kConnect: {
                OnConnect(event.connect());
                break;
            }
            case FfiEvent::kRoomEvent: {
                OnRoomEvent(event.room_event());
                break;
            }
            case FfiEvent::kVideoStreamEvent:
                OnVideoStreamEvent(event.video_stream_event());
                break;
            case FfiEvent::kAudioStreamEvent:
                OnAudioStreamEvent(event.audio_stream_event());
                break;
            case FfiEvent::kGetSessionStats:
                OnGetSessionStatsCallback(event.get_session_stats());
                break;
            case FfiEvent::kPerformRpc:
                OnPerformRpcCallback(event.perform_rpc());
                break;
            case FfiEvent::kRpcMethodInvocation:
                OnRpcMethodInvocationEvent(event.rpc_method_invocation());
                break;
            default:
                break;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception while processing FFI event: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception while processing FFI event" << std::endl;
    }
}

void Room::OnConnect(const ConnectCallback& cb) 
{
    std::lock_guard<std::mutex> guard(lock_);

    // Match the async_id with the pending connectAsyncId_
    if (cb.async_id() != connectAsyncId_) {
        return;
    }

    std::cout << "Received ConnectCallback" << std::endl;

    if (cb.message_case() == ConnectCallback::kError) {
        std::cerr << "Failed to connect to room: " << cb.error() << std::endl;
        connected_ = false;
        return;
    }

    // Success path
    const auto& result = cb.result();
    const auto& owned_room = result.room();
    
    // Store room handle and info
    handle_ = FfiHandle(static_cast<uintptr_t>(owned_room.handle().id()));
    room_info_ = owned_room.info();
    
    // Create local participant
    const auto& local_part = result.local_participant();
    local_participant_ = std::make_shared<LocalParticipant>(
        local_part.info(),
        FfiHandle(static_cast<uintptr_t>(local_part.handle().id()))
    );

    // Create remote participants
    for (const auto& part_with_tracks : result.participants()) {
        const auto& owned_part = part_with_tracks.participant();
        auto remote_part = std::make_shared<RemoteParticipant>(
            owned_part.info(),
            FfiHandle(static_cast<uintptr_t>(owned_part.handle().id()))
        );

        // Add track publications
        for (const auto& owned_pub : part_with_tracks.publications()) {
            auto publication = std::make_shared<RemoteTrackPublication>(
                owned_pub.info(),
                FfiHandle(static_cast<uintptr_t>(owned_pub.handle().id()))
            );
            remote_part->track_publications_[owned_pub.info().sid()] = publication;
        }

        remote_participants_[owned_part.info().identity()] = remote_part;
    }

    connected_ = true;
    connection_state_ = proto::CONN_CONNECTED;
    
    std::cout << "Connected to room: " << room_info_.name() << std::endl;
    std::cout << "Local participant: " << local_participant_->GetIdentity() << std::endl;
    std::cout << "Remote participants: " << remote_participants_.size() << std::endl;
}

void Room::OnRoomEvent(const proto::RoomEvent& event)
{
    switch (event.message_case()) {
        case proto::RoomEvent::kParticipantConnected:
            HandleParticipantConnected(event.participant_connected());
            break;
        case proto::RoomEvent::kParticipantDisconnected:
            HandleParticipantDisconnected(event.participant_disconnected());
            break;
        case proto::RoomEvent::kTrackPublished:
            HandleTrackPublished(event.track_published());
            break;
        case proto::RoomEvent::kTrackUnpublished:
            HandleTrackUnpublished(event.track_unpublished());
            break;
        case proto::RoomEvent::kTrackSubscribed:
            HandleTrackSubscribed(event.track_subscribed());
            break;
        case proto::RoomEvent::kTrackUnsubscribed:
            HandleTrackUnsubscribed(event.track_unsubscribed());
            break;
        case proto::RoomEvent::kTrackMuted:
            HandleTrackMuted(event.track_muted());
            break;
        case proto::RoomEvent::kTrackUnmuted:
            HandleTrackUnmuted(event.track_unmuted());
            break;
        case proto::RoomEvent::kDataPacketReceived:
            HandleDataPacketReceived(event.data_packet_received());
            break;
        case proto::RoomEvent::kConnectionStateChanged:
            HandleConnectionStateChanged(event.connection_state_changed());
            break;
        case proto::RoomEvent::kConnectionQualityChanged:
            HandleConnectionQualityChanged(event.connection_quality_changed());
            break;
        case proto::RoomEvent::kParticipantMetadataChanged:
            HandleParticipantMetadataChanged(event.participant_metadata_changed());
            break;
        case proto::RoomEvent::kParticipantNameChanged:
            HandleParticipantNameChanged(event.participant_name_changed());
            break;
        case proto::RoomEvent::kActiveSpeakersChanged:
            HandleActiveSpeakersChanged(event.active_speakers_changed());
            break;
        case proto::RoomEvent::kRoomMetadataChanged:
            HandleRoomMetadataChanged(event.room_metadata_changed());
            break;
        case proto::RoomEvent::kChatMessage:
            HandleChatMessageReceived(event.chat_message());
            break;
        case proto::RoomEvent::kTranscriptionReceived:
            HandleTranscriptionReceived(event.transcription_received());
            break;
        default:
            break;
    }
}

void Room::OnVideoStreamEvent(const proto::VideoStreamEvent& event)
{
    VideoStreamCallback cb;
    std::string participant;
    std::string track_sid;
    {
        std::lock_guard<std::mutex> g(lock_);
        auto it = video_streams_.find(event.stream_handle());
        if (it == video_streams_.end()) {
            return;
        }
        cb = it->second.callback;
        participant = it->second.participant_identity;
        track_sid = it->second.track_sid;
    }

    if (!cb) {
        if (event.message_case() == proto::VideoStreamEvent::kFrameReceived) {
            FfiHandle buffer_handle(static_cast<uintptr_t>(event.frame_received().buffer().handle().id()));
        }
        return;
    }

    switch (event.message_case()) {
        case proto::VideoStreamEvent::kFrameReceived: {
            const auto& frame_msg = event.frame_received();
            const auto& buffer = frame_msg.buffer();
            VideoFrame frame;
            frame.type = buffer.info().type();
            frame.width = buffer.info().width();
            frame.height = buffer.info().height();
            frame.rotation = frame_msg.rotation();
            frame.timestamp_us = frame_msg.timestamp_us();

            FfiHandle buffer_handle(static_cast<uintptr_t>(buffer.handle().id()));
            const uint8_t* data_ptr = reinterpret_cast<const uint8_t*>(buffer.info().data_ptr());
            if (data_ptr && frame.width > 0 && frame.height > 0) {
                uint32_t stride = buffer.info().stride();
                if (stride == 0) {
                    stride = frame.width * 4;
                }
                size_t data_size = static_cast<size_t>(stride) * frame.height;

                frame.data.assign(data_ptr, data_ptr + data_size);
            }

            cb(frame, participant, track_sid);
            break;
        }
        case proto::VideoStreamEvent::kEos:
            RemoveVideoStream(event.stream_handle());
            break;
        default:
            break;
    }
}

void Room::OnAudioStreamEvent(const proto::AudioStreamEvent& event)
{
    AudioStreamCallback cb;
    std::string participant;
    std::string track_sid;
    {
        std::lock_guard<std::mutex> g(lock_);
        auto it = audio_streams_.find(event.stream_handle());
        if (it == audio_streams_.end()) {
            return;
        }
        cb = it->second.callback;
        participant = it->second.participant_identity;
        track_sid = it->second.track_sid;
    }

    if (!cb) {
        if (event.message_case() == proto::AudioStreamEvent::kFrameReceived) {
            FfiHandle buffer_handle(static_cast<uintptr_t>(event.frame_received().frame().handle().id()));
        }
        return;
    }

    switch (event.message_case()) {
        case proto::AudioStreamEvent::kFrameReceived: {
            const auto& frame_msg = event.frame_received().frame();
            AudioFrame frame;
            frame.sample_rate = frame_msg.info().sample_rate();
            frame.num_channels = frame_msg.info().num_channels();
            frame.samples_per_channel = frame_msg.info().samples_per_channel();

            FfiHandle buffer_handle(static_cast<uintptr_t>(frame_msg.handle().id()));
            const int16_t* samples = reinterpret_cast<const int16_t*>(frame_msg.info().data_ptr());
            size_t sample_count = static_cast<size_t>(frame.num_channels) * frame.samples_per_channel;
            if (samples && sample_count > 0) {
                frame.samples.assign(samples, samples + sample_count);
            }

            cb(frame, participant, track_sid);
            break;
        }
        case proto::AudioStreamEvent::kEos:
            RemoveAudioStream(event.stream_handle());
            break;
        default:
            break;
    }
}

// ============================================================================
// Room Event Handlers
// ============================================================================

void Room::HandleParticipantConnected(const proto::ParticipantConnected& event)
{
    const auto& owned_part = event.info();
    auto participant = std::make_shared<RemoteParticipant>(
        owned_part.info(),
        FfiHandle(static_cast<uintptr_t>(owned_part.handle().id()))
    );

    ParticipantConnectedCallback cb;
    {
        std::lock_guard<std::mutex> g(lock_);
        remote_participants_[owned_part.info().identity()] = participant;
        cb = on_participant_connected_;
    }

    std::cout << "Participant connected: " << participant->GetIdentity() << std::endl;

    if (cb) {
        cb(participant);
    }
}

void Room::HandleParticipantDisconnected(const proto::ParticipantDisconnected& event)
{
    ParticipantDisconnectedCallback cb;
    std::vector<std::string> track_sids;
    std::string identity = event.participant_identity();

    {
        std::lock_guard<std::mutex> g(lock_);
        auto it = remote_participants_.find(identity);
        if (it == remote_participants_.end()) {
            return;
        }

        for (const auto& pub : it->second->GetTracks()) {
            if (pub) {
                track_sids.push_back(pub->GetSid());
            }
        }

        cb = on_participant_disconnected_;
        remote_participants_.erase(it);
    }

    std::cout << "Participant disconnected: " << identity << std::endl;

    for (const auto& sid : track_sids) {
        RemoveStreamsForTrack(sid);
    }

    if (cb) {
        cb(identity, event.disconnect_reason());
    }
}

void Room::HandleTrackPublished(const proto::TrackPublished& event)
{
    std::shared_ptr<RemoteParticipant> participant;
    std::shared_ptr<RemoteTrackPublication> publication;
    TrackPublishedCallback cb;

    {
        std::lock_guard<std::mutex> g(lock_);
        auto it = remote_participants_.find(event.participant_identity());
        if (it == remote_participants_.end()) {
            return;
        }

        participant = it->second;
        const auto& owned_pub = event.publication();
        publication = std::make_shared<RemoteTrackPublication>(
            owned_pub.info(),
            FfiHandle(static_cast<uintptr_t>(owned_pub.handle().id()))
        );

        participant->track_publications_[owned_pub.info().sid()] = publication;
        cb = on_track_published_;
    }

    std::cout << "Track published: " << event.publication().info().name() 
              << " by " << event.participant_identity() << std::endl;

    if (cb) {
        cb(publication, participant);
    }
}

void Room::HandleTrackUnpublished(const proto::TrackUnpublished& event)
{
    std::shared_ptr<RemoteParticipant> participant;
    TrackUnpublishedCallback cb;
    std::string publication_sid = event.publication_sid();

    {
        std::lock_guard<std::mutex> g(lock_);
        auto it = remote_participants_.find(event.participant_identity());
        if (it == remote_participants_.end()) {
            return;
        }

        participant = it->second;
        participant->track_publications_.erase(publication_sid);
        cb = on_track_unpublished_;
    }

    std::cout << "Track unpublished: " << publication_sid 
              << " by " << event.participant_identity() << std::endl;

    RemoveStreamsForTrack(publication_sid);

    if (cb) {
        cb(publication_sid, participant);
    }
}

void Room::HandleTrackSubscribed(const proto::TrackSubscribed& event)
{
    std::shared_ptr<RemoteParticipant> participant;
    std::shared_ptr<RemoteTrackPublication> publication;
    std::shared_ptr<RemoteTrack> track;
    TrackSubscribedCallback cb;

    {
        std::lock_guard<std::mutex> g(lock_);
        auto it = remote_participants_.find(event.participant_identity());
        if (it == remote_participants_.end()) {
            return;
        }

        participant = it->second;
        const auto& owned_track = event.track();
        track = std::make_shared<RemoteTrack>(
            owned_track.info(),
            FfiHandle(static_cast<uintptr_t>(owned_track.handle().id()))
        );

        auto pub_it = participant->track_publications_.find(owned_track.info().sid());
        if (pub_it != participant->track_publications_.end()) {
            publication = std::dynamic_pointer_cast<RemoteTrackPublication>(pub_it->second);
            if (publication) {
                publication->SetTrack(track);
            }
        }

        cb = on_track_subscribed_;
    }

    if (!track) {
        return;
    }

    std::cout << "Track subscribed: " << event.track().info().name() 
              << " from " << event.participant_identity() << std::endl;

    if (cb && publication) {
        cb(track, publication, participant);
    }
}

void Room::HandleTrackUnsubscribed(const proto::TrackUnsubscribed& event)
{
    std::shared_ptr<RemoteParticipant> participant;
    TrackUnsubscribedCallback cb;
    std::string track_sid = event.track_sid();

    {
        std::lock_guard<std::mutex> g(lock_);
        auto it = remote_participants_.find(event.participant_identity());
        if (it == remote_participants_.end()) {
            return;
        }

        participant = it->second;

        auto pub_it = participant->track_publications_.find(track_sid);
        if (pub_it != participant->track_publications_.end()) {
            auto publication = std::dynamic_pointer_cast<RemoteTrackPublication>(pub_it->second);
            if (publication) {
                publication->SetTrack(nullptr);
            }
        }

        cb = on_track_unsubscribed_;
    }

    RemoveStreamsForTrack(track_sid);

    std::cout << "Track unsubscribed: " << track_sid 
              << " from " << event.participant_identity() << std::endl;

    if (cb) {
        cb(track_sid, participant);
    }
}

void Room::HandleTrackMuted(const proto::TrackMuted& event)
{
    auto participant = GetParticipant(event.participant_identity());
    if (!participant) return;

    std::cout << "Track muted: " << event.track_sid() 
              << " from " << event.participant_identity() << std::endl;

    TrackMutedCallback cb;
    {
        std::lock_guard<std::mutex> g(lock_);
        cb = on_track_muted_;
    }
    if (cb) {
        cb(event.track_sid(), event.participant_identity(), true);
    }
}

void Room::HandleTrackUnmuted(const proto::TrackUnmuted& event)
{
    auto participant = GetParticipant(event.participant_identity());
    if (!participant) return;

    std::cout << "Track unmuted: " << event.track_sid() 
              << " from " << event.participant_identity() << std::endl;

    TrackMutedCallback cb;
    {
        std::lock_guard<std::mutex> g(lock_);
        cb = on_track_muted_;
    }
    if (cb) {
        cb(event.track_sid(), event.participant_identity(), false);
    }
}

void Room::HandleDataPacketReceived(const proto::DataPacketReceived& event)
{
    DataReceivedCallback cb;
    {
        std::lock_guard<std::mutex> g(lock_);
        cb = on_data_received_;
    }

    if (event.has_user()) {
        const auto& user_packet = event.user();
        const auto& buffer = user_packet.data();
        
        // Convert buffer to vector and ensure handle is released
        std::vector<uint8_t> data;
        {
            FfiHandle buffer_handle(static_cast<uintptr_t>(buffer.handle().id()));
            const auto* ptr = reinterpret_cast<const uint8_t*>(buffer.data().data_ptr());
            size_t len = static_cast<size_t>(buffer.data().data_len());
            if (ptr && len > 0) {
                data.assign(ptr, ptr + len);
            }
        }
        
        std::string topic = user_packet.has_topic() ? user_packet.topic() : "";

        std::cout << "Data received from " << event.participant_identity() 
                  << " (topic: " << topic << ", size: " << data.size() << ")" << std::endl;

        if (cb) {
            cb(data, event.participant_identity(), topic);
        }
    }
}

void Room::HandleConnectionStateChanged(const proto::ConnectionStateChanged& event)
{
    ConnectionStateChangedCallback cb;
    {
        std::lock_guard<std::mutex> g(lock_);
        connection_state_ = event.state();

        if (connection_state_ == proto::CONN_DISCONNECTED) {
            connected_ = false;
            video_streams_.clear();
            audio_streams_.clear();
            track_video_streams_.clear();
            track_audio_streams_.clear();
        }

        cb = on_connection_state_changed_;
    }

    std::cout << "Connection state changed: " << static_cast<int>(connection_state_) << std::endl;

    if (cb) {
        cb(connection_state_);
    }
}

void Room::HandleConnectionQualityChanged(const proto::ConnectionQualityChanged& event)
{
    auto participant = GetParticipant(event.participant_identity());
    if (participant) {
        participant->SetConnectionQuality(event.quality());
        
        std::cout << "Connection quality changed for " << event.participant_identity() 
                  << ": " << static_cast<int>(event.quality()) << std::endl;
    }
}

void Room::HandleParticipantMetadataChanged(const proto::ParticipantMetadataChanged& event)
{
    auto participant = GetParticipant(event.participant_identity());
    if (participant) {
        proto::ParticipantInfo updated_info = participant->info_;
        updated_info.set_metadata(event.metadata());
        participant->UpdateInfo(updated_info);

        std::cout << "Participant metadata changed: " << event.participant_identity() << std::endl;
    }
}

void Room::HandleParticipantNameChanged(const proto::ParticipantNameChanged& event)
{
    auto participant = GetParticipant(event.participant_identity());
    if (participant) {
        proto::ParticipantInfo updated_info = participant->info_;
        updated_info.set_name(event.name());
        participant->UpdateInfo(updated_info);

        std::cout << "Participant name changed: " << event.participant_identity() 
                  << " -> " << event.name() << std::endl;
    }
}

void Room::HandleActiveSpeakersChanged(const proto::ActiveSpeakersChanged& event)
{
    ActiveSpeakersChangedCallback cb;
    std::vector<std::string> speakers;
    
    for (const auto& identity : event.participant_identities()) {
        speakers.push_back(identity);
    }
    
    {
        std::lock_guard<std::mutex> g(lock_);
        active_speakers_ = speakers;
        cb = on_active_speakers_changed_;
    }

    std::cout << "Active speakers changed: " << speakers.size() << " speakers" << std::endl;

    if (cb) {
        cb(speakers);
    }
}

void Room::HandleRoomMetadataChanged(const proto::RoomMetadataChanged& event)
{
    RoomMetadataChangedCallback cb;
    {
        std::lock_guard<std::mutex> g(lock_);
        room_info_.set_metadata(event.metadata());
        cb = on_room_metadata_changed_;
    }

    std::cout << "Room metadata changed" << std::endl;

    if (cb) {
        cb(event.metadata());
    }
}

void Room::HandleChatMessageReceived(const proto::ChatMessageReceived& event)
{
    ChatMessageReceivedCallback cb;
    {
        std::lock_guard<std::mutex> g(lock_);
        cb = on_chat_message_received_;
    }

    if (cb) {
        const auto& msg = event.message();
        ChatMessage chat_msg;
        chat_msg.id = msg.id();
        chat_msg.timestamp = msg.timestamp();
        chat_msg.message = msg.message();
        chat_msg.sender_identity = event.participant_identity();
        chat_msg.edit_timestamp = msg.edit_timestamp();
        chat_msg.deleted = msg.deleted();
        chat_msg.generated = msg.generated();

        std::cout << "Chat message received from " << event.participant_identity() 
                  << ": " << chat_msg.message << std::endl;

        cb(chat_msg);
    }
}

void Room::HandleTranscriptionReceived(const proto::TranscriptionReceived& event)
{
    TranscriptionReceivedCallback cb;
    {
        std::lock_guard<std::mutex> g(lock_);
        cb = on_transcription_received_;
    }

    if (cb) {
        std::vector<TranscriptionSegment> segments;
        for (const auto& seg : event.segments()) {
            TranscriptionSegment ts;
            ts.id = seg.id();
            ts.text = seg.text();
            ts.start_time = seg.start_time();
            ts.end_time = seg.end_time();
            ts.final = seg.final();
            ts.language = seg.language();
            segments.push_back(std::move(ts));
        }

        std::cout << "Transcription received from " << event.participant_identity() 
                  << ": " << segments.size() << " segments" << std::endl;

        cb(segments, event.participant_identity(), event.track_sid());
    }
}

} // namespace livekit
