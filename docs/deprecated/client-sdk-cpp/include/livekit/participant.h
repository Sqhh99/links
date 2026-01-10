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

#ifndef LIVEKIT_PARTICIPANT_H
#define LIVEKIT_PARTICIPANT_H

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <cstdint>
#include "ffi_client.h"
#include "participant.pb.h"
#include "room.pb.h"
#include "track.pb.h"

namespace livekit
{
    // Forward declarations
    class TrackPublication;
    class LocalTrackPublication;
    class LocalTrack;

    // Enums from protobuf
    using ParticipantKind = proto::ParticipantKind;
    using DisconnectReason = proto::DisconnectReason;

    // Base Participant class
    class Participant
    {
    public:
        virtual ~Participant() = default;

        // Identity and metadata
        const std::string& GetIdentity() const { return info_.identity(); }
        const std::string& GetSid() const { return info_.sid(); }
        const std::string& GetName() const { return info_.name(); }
        const std::string& GetMetadata() const { return info_.metadata(); }
        ParticipantKind GetKind() const { return info_.kind(); }

        // Attributes
        const std::map<std::string, std::string>& GetAttributes() const { return attributes_; }
        std::string GetAttribute(const std::string& key) const;

        // Track publications
        std::vector<std::shared_ptr<TrackPublication>> GetTracks() const;
        std::shared_ptr<TrackPublication> GetTrack(const std::string& sid) const;

        // Get handle for FFI calls
        uintptr_t GetHandleId() const { return handle_.handle; }

    protected:
        Participant(const proto::ParticipantInfo& info, FfiHandle handle);
        
        proto::ParticipantInfo info_;
        FfiHandle handle_;
        std::map<std::string, std::string> attributes_;
        std::map<std::string, std::shared_ptr<TrackPublication>> track_publications_;

        friend class Room;
    };

    // Local participant (this client)
    class LocalParticipant : public Participant
    {
    public:
        LocalParticipant(const proto::ParticipantInfo& info, FfiHandle handle);
        ~LocalParticipant() override = default;

        // Publish local tracks
        std::shared_ptr<LocalTrackPublication> PublishTrack(
            std::shared_ptr<LocalTrack> track,
            const proto::TrackPublishOptions& options);

        void UnpublishTrack(const std::string& track_sid);

        // Update metadata
        void SetMetadata(const std::string& metadata);
        void SetName(const std::string& name);
        void SetAttributes(const std::map<std::string, std::string>& attributes);

        // Data publishing
        void PublishData(const std::vector<uint8_t>& data, bool reliable, 
                        const std::string& topic = "",
                        const std::vector<std::string>& destination_identities = {});

    private:
        void UpdateInfo(const proto::ParticipantInfo& info);
        
        friend class Room;
    };

    // Remote participant (other clients)
    class RemoteParticipant : public Participant
    {
    public:
        RemoteParticipant(const proto::ParticipantInfo& info, FfiHandle handle);
        ~RemoteParticipant() override = default;

        // Connection quality
        proto::ConnectionQuality GetConnectionQuality() const { return connection_quality_; }

    private:
        void UpdateInfo(const proto::ParticipantInfo& info);
        void SetConnectionQuality(proto::ConnectionQuality quality);

        proto::ConnectionQuality connection_quality_;

        friend class Room;
    };
}

#endif /* LIVEKIT_PARTICIPANT_H */
