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

#ifndef LIVEKIT_ROOM_H
#define LIVEKIT_ROOM_H

#include <mutex>
#include <memory>
#include <map>
#include <vector>
#include <functional>
#include <unordered_map>
#include "ffi.pb.h"
#include "room.pb.h"
#include "livekit/ffi_client.h"
#include "livekit/participant.h"
#include "livekit/track.h"

namespace livekit
{
    // Enums from protobuf
    using ConnectionState = proto::ConnectionState;
    using ConnectionQuality = proto::ConnectionQuality;

    // Event callback types
    using ParticipantConnectedCallback = std::function<void(std::shared_ptr<RemoteParticipant>)>;
    using ParticipantDisconnectedCallback = std::function<void(const std::string& identity, DisconnectReason reason)>;
    using TrackPublishedCallback = std::function<void(std::shared_ptr<RemoteTrackPublication>, std::shared_ptr<RemoteParticipant>)>;
    using TrackUnpublishedCallback = std::function<void(const std::string& publication_sid, std::shared_ptr<RemoteParticipant>)>;
    using TrackSubscribedCallback = std::function<void(std::shared_ptr<RemoteTrack>, std::shared_ptr<RemoteTrackPublication>, std::shared_ptr<RemoteParticipant>)>;
    using TrackUnsubscribedCallback = std::function<void(const std::string& track_sid, std::shared_ptr<RemoteParticipant>)>;
    using TrackMutedCallback = std::function<void(const std::string& track_sid, const std::string& participant_identity, bool muted)>;
    using DataReceivedCallback = std::function<void(const std::vector<uint8_t>& data, const std::string& participant_identity, const std::string& topic)>;
    using ConnectionStateChangedCallback = std::function<void(ConnectionState state)>;
    using ActiveSpeakersChangedCallback = std::function<void(const std::vector<std::string>& participant_identities)>;
    using RoomMetadataChangedCallback = std::function<void(const std::string& metadata)>;

    // Chat message structure
    struct ChatMessage {
        std::string id;
        int64_t timestamp;
        std::string message;
        std::string sender_identity;
        int64_t edit_timestamp{0};
        bool deleted{false};
        bool generated{false};
    };

    using ChatMessageReceivedCallback = std::function<void(const ChatMessage& message)>;

    // Transcription segment
    struct TranscriptionSegment {
        std::string id;
        std::string text;
        uint64_t start_time;
        uint64_t end_time;
        bool final;
        std::string language;
    };

    using TranscriptionReceivedCallback = std::function<void(const std::vector<TranscriptionSegment>& segments, 
                                                              const std::string& participant_identity,
                                                              const std::string& track_sid)>;

    // Session stats
    struct SessionStats {
        std::vector<proto::RtcStats> publisher_stats;
        std::vector<proto::RtcStats> subscriber_stats;
    };

    // RPC error
    struct RpcError {
        uint32_t code{0};
        std::string message;
        std::string data;
    };

    // RPC method invocation
    struct RpcMethodInvocation {
        uint64_t invocation_id;
        std::string method;
        std::string request_id;
        std::string caller_identity;
        std::string payload;
        uint32_t response_timeout_ms;
    };

    // RPC method handler - returns optional payload or error
    using RpcMethodHandler = std::function<void(const RpcMethodInvocation& invocation,
                                                 std::function<void(const std::string& payload)> respond,
                                                 std::function<void(const RpcError& error)> respond_error)>;

    // RPC response callback
    using RpcResponseCallback = std::function<void(const std::string& payload, const RpcError* error)>;

    struct VideoFrame {
        std::vector<uint8_t> data;
        uint32_t width{0};
        uint32_t height{0};
        proto::VideoBufferType type{proto::RGBA};
        proto::VideoRotation rotation{proto::VIDEO_ROTATION_0};
        int64_t timestamp_us{0};
    };

    struct AudioFrame {
        std::vector<int16_t> samples;
        uint32_t sample_rate{48000};
        uint32_t num_channels{1};
        uint32_t samples_per_channel{0};
    };

    using VideoStreamCallback = std::function<void(const VideoFrame& frame, const std::string& participant_identity, const std::string& track_sid)>;
    using AudioStreamCallback = std::function<void(const AudioFrame& frame, const std::string& participant_identity, const std::string& track_sid)>;

    class Room
    {
    public:
        Room();
        ~Room();

        // Connection
        void Connect(const std::string& url, const std::string& token);
        void Disconnect();

        // Participant access
        std::shared_ptr<LocalParticipant> GetLocalParticipant() const;
        std::vector<std::shared_ptr<RemoteParticipant>> GetRemoteParticipants() const;
        std::shared_ptr<RemoteParticipant> GetParticipant(const std::string& identity) const;

        // Room info
        const proto::RoomInfo& GetRoomInfo() const { return room_info_; }
        ConnectionState GetConnectionState() const { return connection_state_; }
        bool IsConnected() const { return connected_; }

        // Event callbacks
        void SetOnParticipantConnected(ParticipantConnectedCallback callback);
        void SetOnParticipantDisconnected(ParticipantDisconnectedCallback callback);
        void SetOnTrackPublished(TrackPublishedCallback callback);
        void SetOnTrackUnpublished(TrackUnpublishedCallback callback);
        void SetOnTrackSubscribed(TrackSubscribedCallback callback);
        void SetOnTrackUnsubscribed(TrackUnsubscribedCallback callback);
        void SetOnTrackMuted(TrackMutedCallback callback);
        void SetOnDataReceived(DataReceivedCallback callback);
        void SetOnConnectionStateChanged(ConnectionStateChangedCallback callback);
        void SetOnActiveSpeakersChanged(ActiveSpeakersChangedCallback callback);
        void SetOnRoomMetadataChanged(RoomMetadataChangedCallback callback);
        void SetOnChatMessageReceived(ChatMessageReceivedCallback callback);
        void SetOnTranscriptionReceived(TranscriptionReceivedCallback callback);

        // Chat messaging
        void SendChatMessage(const std::string& message,
                            const std::vector<std::string>& destination_identities = {});
        void EditChatMessage(const std::string& edit_text,
                            const std::string& original_message_id,
                            int64_t original_timestamp,
                            const std::string& original_message,
                            const std::vector<std::string>& destination_identities = {});

        // Session statistics
        void GetSessionStats(std::function<void(const SessionStats& stats, const std::string& error)> callback);

        // Active speakers
        std::vector<std::string> GetActiveSpeakers() const;

        // RPC
        void RegisterRpcMethod(const std::string& method, RpcMethodHandler handler);
        void UnregisterRpcMethod(const std::string& method);
        void PerformRpc(const std::string& destination_identity,
                       const std::string& method,
                       const std::string& payload,
                       RpcResponseCallback callback,
                       uint32_t response_timeout_ms = 10000);

        // Media stream helpers
        uint64_t CreateVideoStream(std::shared_ptr<RemoteTrack> track,
                                   const std::string& participant_identity,
                                   VideoStreamCallback callback,
                                   proto::VideoBufferType format = proto::RGBA,
                                   bool normalize_stride = true);
        uint64_t CreateAudioStream(std::shared_ptr<RemoteTrack> track,
                                   const std::string& participant_identity,
                                   AudioStreamCallback callback,
                                   uint32_t sample_rate = 48000,
                                   uint32_t num_channels = 1);
        void RemoveVideoStream(uint64_t stream_handle);
        void RemoveAudioStream(uint64_t stream_handle);
        void RemoveStreamsForTrack(const std::string& track_sid);

    private:
        void OnConnect(const proto::ConnectCallback& cb);
        void OnEvent(const proto::FfiEvent& event);
        void OnRoomEvent(const proto::RoomEvent& event);
        void OnVideoStreamEvent(const proto::VideoStreamEvent& event);
        void OnAudioStreamEvent(const proto::AudioStreamEvent& event);
        void OnGetSessionStatsCallback(const proto::GetSessionStatsCallback& cb);
        void OnPerformRpcCallback(const proto::PerformRpcCallback& cb);
        void OnRpcMethodInvocationEvent(const proto::RpcMethodInvocationEvent& event);
        void RespondToRpcInvocation(uint64_t invocation_id, const std::string& payload);
        void RespondToRpcInvocationWithError(uint64_t invocation_id, const RpcError& error);

        // Room event handlers
        void HandleParticipantConnected(const proto::ParticipantConnected& event);
        void HandleParticipantDisconnected(const proto::ParticipantDisconnected& event);
        void HandleTrackPublished(const proto::TrackPublished& event);
        void HandleTrackUnpublished(const proto::TrackUnpublished& event);
        void HandleTrackSubscribed(const proto::TrackSubscribed& event);
        void HandleTrackUnsubscribed(const proto::TrackUnsubscribed& event);
        void HandleTrackMuted(const proto::TrackMuted& event);
        void HandleTrackUnmuted(const proto::TrackUnmuted& event);
        void HandleDataPacketReceived(const proto::DataPacketReceived& event);
        void HandleConnectionStateChanged(const proto::ConnectionStateChanged& event);
        void HandleConnectionQualityChanged(const proto::ConnectionQualityChanged& event);
        void HandleParticipantMetadataChanged(const proto::ParticipantMetadataChanged& event);
        void HandleParticipantNameChanged(const proto::ParticipantNameChanged& event);
        void HandleActiveSpeakersChanged(const proto::ActiveSpeakersChanged& event);
        void HandleRoomMetadataChanged(const proto::RoomMetadataChanged& event);
        void HandleChatMessageReceived(const proto::ChatMessageReceived& event);
        void HandleTranscriptionReceived(const proto::TranscriptionReceived& event);
        void RemoveStreamsForTrackLocked(const std::string& track_sid);

        mutable std::mutex lock_;
        FfiHandle handle_{INVALID_HANDLE};
        FfiClient::ListenerId listener_id_{0};
        bool connected_{false};
        uint64_t connectAsyncId_{0};
        
        // Room state
        proto::RoomInfo room_info_;
        ConnectionState connection_state_{proto::CONN_DISCONNECTED};
        
        // Participants
        std::shared_ptr<LocalParticipant> local_participant_;
        std::map<std::string, std::shared_ptr<RemoteParticipant>> remote_participants_;

        // Event callbacks
        ParticipantConnectedCallback on_participant_connected_;
        ParticipantDisconnectedCallback on_participant_disconnected_;
        TrackPublishedCallback on_track_published_;
        TrackUnpublishedCallback on_track_unpublished_;
        TrackSubscribedCallback on_track_subscribed_;
        TrackUnsubscribedCallback on_track_unsubscribed_;
        TrackMutedCallback on_track_muted_;
        DataReceivedCallback on_data_received_;
        ConnectionStateChangedCallback on_connection_state_changed_;
        ActiveSpeakersChangedCallback on_active_speakers_changed_;
        RoomMetadataChangedCallback on_room_metadata_changed_;
        ChatMessageReceivedCallback on_chat_message_received_;
        TranscriptionReceivedCallback on_transcription_received_;

        // Active speakers state
        std::vector<std::string> active_speakers_;

        // Async callbacks
        std::unordered_map<uint64_t, std::function<void(const SessionStats&, const std::string&)>> session_stats_callbacks_;
        std::unordered_map<uint64_t, std::function<void(const ChatMessage&, const std::string&)>> chat_message_callbacks_;

        // RPC state
        std::unordered_map<std::string, RpcMethodHandler> rpc_method_handlers_;
        std::unordered_map<uint64_t, RpcResponseCallback> rpc_response_callbacks_;

        struct VideoStreamState {
            FfiHandle handle;
            std::string track_sid;
            std::string participant_identity;
            VideoStreamCallback callback;
        };

        struct AudioStreamState {
            FfiHandle handle;
            std::string track_sid;
            std::string participant_identity;
            AudioStreamCallback callback;
        };

        std::unordered_map<uint64_t, VideoStreamState> video_streams_;
        std::unordered_map<uint64_t, AudioStreamState> audio_streams_;
        std::unordered_map<std::string, uint64_t> track_video_streams_;
        std::unordered_map<std::string, uint64_t> track_audio_streams_;
    };
}

#endif /* LIVEKIT_ROOM_H */
