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

#ifndef LIVEKIT_TRACK_H
#define LIVEKIT_TRACK_H

#include <string>
#include <memory>
#include <functional>
#include <cstdint>
#include "ffi_client.h"
#include "track.pb.h"
#include "track_publication.pb.h"

namespace livekit
{
    // Enums from protobuf
    using TrackKind = proto::TrackKind;
    using TrackSource = proto::TrackSource;
    using StreamState = proto::StreamState;
    using VideoQuality = proto::VideoQuality;

    // Forward declarations
    class Track;
    class LocalTrack;
    class RemoteTrack;

    // Base Track class
    class Track
    {
    public:
        virtual ~Track() = default;

        const std::string& GetSid() const { return info_.sid(); }
        const std::string& GetName() const { return info_.name(); }
        TrackKind GetKind() const { return info_.kind(); }
        StreamState GetStreamState() const { return info_.stream_state(); }
        bool IsMuted() const { return info_.muted(); }
        bool IsRemote() const { return info_.remote(); }

        const FfiHandle& GetHandle() const { return handle_; }

    protected:
        Track(const proto::TrackInfo& info, FfiHandle handle);

        proto::TrackInfo info_;
        FfiHandle handle_;

        friend class Room;
        friend class TrackPublication;
    };

    // Local track (published by this client)
    class LocalTrack : public Track
    {
    public:
        LocalTrack(const proto::TrackInfo& info, FfiHandle handle);
        ~LocalTrack() override = default;

        void Mute();
        void Unmute();

    private:
        void UpdateMuteState(bool muted);

        friend class Room;
    };

    // Remote track (published by other participants)
    class RemoteTrack : public Track
    {
    public:
        RemoteTrack(const proto::TrackInfo& info, FfiHandle handle);
        ~RemoteTrack() override = default;

        void SetEnabled(bool enabled);

    private:
        bool enabled_;

        friend class Room;
    };

    // Track Publication base class
    class TrackPublication
    {
    public:
        virtual ~TrackPublication() = default;

        const std::string& GetSid() const { return info_.sid(); }
        const std::string& GetName() const { return info_.name(); }
        TrackKind GetKind() const { return info_.kind(); }
        TrackSource GetSource() const { return info_.source(); }
        bool IsMuted() const { return info_.muted(); }
        bool IsRemote() const { return info_.remote(); }
        
        uint32_t GetWidth() const { return info_.width(); }
        uint32_t GetHeight() const { return info_.height(); }
        const std::string& GetMimeType() const { return info_.mime_type(); }

        std::shared_ptr<Track> GetTrack() const { return track_; }
        const FfiHandle& GetHandle() const { return handle_; }

    protected:
        TrackPublication(const proto::TrackPublicationInfo& info, FfiHandle handle);

        proto::TrackPublicationInfo info_;
        FfiHandle handle_;
        std::shared_ptr<Track> track_;

        friend class Room;
        friend class Participant;
    };

    // Local track publication
    class LocalTrackPublication : public TrackPublication
    {
    public:
        LocalTrackPublication(const proto::TrackPublicationInfo& info, FfiHandle handle);
        ~LocalTrackPublication() override = default;

        void Mute();
        void Unmute();

    private:
        friend class LocalParticipant;
    };

    // Remote track publication
    class RemoteTrackPublication : public TrackPublication
    {
    public:
        RemoteTrackPublication(const proto::TrackPublicationInfo& info, FfiHandle handle);
        ~RemoteTrackPublication() override = default;

        void SetSubscribed(bool subscribed);
        void SetVideoQuality(VideoQuality quality);
        void SetVideoDimension(uint32_t width, uint32_t height);
        void SetEnabled(bool enabled);

        bool IsSubscribed() const { return track_ != nullptr; }

    private:
        void SetTrack(std::shared_ptr<RemoteTrack> track);

        friend class Room;
    };
}

#endif /* LIVEKIT_TRACK_H */
