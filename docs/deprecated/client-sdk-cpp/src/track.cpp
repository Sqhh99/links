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

#include "livekit/track.h"
#include "livekit/ffi_client.h"
#include "track_publication.pb.h"
#include <iostream>

namespace livekit
{

// ============================================================================
// Track Base Class
// ============================================================================

Track::Track(const proto::TrackInfo& info, FfiHandle handle)
    : info_(info), handle_(std::move(handle))
{
}

// ============================================================================
// LocalTrack
// ============================================================================

LocalTrack::LocalTrack(const proto::TrackInfo& info, FfiHandle handle)
    : Track(info, std::move(handle))
{
}

void LocalTrack::Mute()
{
    proto::FfiRequest req;
    auto* mute = req.mutable_local_track_mute();
    mute->set_track_handle(static_cast<uint64_t>(handle_));
    mute->set_mute(true);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_local_track_mute()) {
        throw std::runtime_error("Invalid response for LocalTrackMute");
    }

    info_.set_muted(resp.local_track_mute().muted());
}

void LocalTrack::Unmute()
{
    proto::FfiRequest req;
    auto* mute = req.mutable_local_track_mute();
    mute->set_track_handle(static_cast<uint64_t>(handle_));
    mute->set_mute(false);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_local_track_mute()) {
        throw std::runtime_error("Invalid response for LocalTrackMute");
    }

    info_.set_muted(resp.local_track_mute().muted());
}

void LocalTrack::UpdateMuteState(bool muted)
{
    info_.set_muted(muted);
}

// ============================================================================
// RemoteTrack
// ============================================================================

RemoteTrack::RemoteTrack(const proto::TrackInfo& info, FfiHandle handle)
    : Track(info, std::move(handle)), enabled_(true)
{
}

void RemoteTrack::SetEnabled(bool enabled)
{
    proto::FfiRequest req;
    auto* enable = req.mutable_enable_remote_track();
    enable->set_track_handle(static_cast<uint64_t>(handle_));
    enable->set_enabled(enabled);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_enable_remote_track()) {
        throw std::runtime_error("Invalid response for EnableRemoteTrack");
    }

    enabled_ = resp.enable_remote_track().enabled();
}

// ============================================================================
// TrackPublication Base Class
// ============================================================================

TrackPublication::TrackPublication(const proto::TrackPublicationInfo& info, FfiHandle handle)
    : info_(info), handle_(std::move(handle)), track_(nullptr)
{
}

// ============================================================================
// LocalTrackPublication
// ============================================================================

LocalTrackPublication::LocalTrackPublication(const proto::TrackPublicationInfo& info, FfiHandle handle)
    : TrackPublication(info, std::move(handle))
{
}

void LocalTrackPublication::Mute()
{
    if (track_) {
        auto local_track = std::dynamic_pointer_cast<LocalTrack>(track_);
        if (local_track) {
            local_track->Mute();
        }
    }
}

void LocalTrackPublication::Unmute()
{
    if (track_) {
        auto local_track = std::dynamic_pointer_cast<LocalTrack>(track_);
        if (local_track) {
            local_track->Unmute();
        }
    }
}

// ============================================================================
// RemoteTrackPublication
// ============================================================================

RemoteTrackPublication::RemoteTrackPublication(const proto::TrackPublicationInfo& info, FfiHandle handle)
    : TrackPublication(info, std::move(handle))
{
}

void RemoteTrackPublication::SetSubscribed(bool subscribed)
{
    proto::FfiRequest req;
    auto* set_sub = req.mutable_set_subscribed();
    set_sub->set_publication_handle(static_cast<uint64_t>(handle_));
    set_sub->set_subscribe(subscribed);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_set_subscribed()) {
        throw std::runtime_error("Invalid response for SetSubscribed");
    }

    // The actual track will be received via TrackSubscribed event
}

void RemoteTrackPublication::SetVideoQuality(VideoQuality quality)
{
    proto::FfiRequest req;
    auto* set_quality = req.mutable_set_remote_track_publication_quality();
    set_quality->set_track_publication_handle(static_cast<uint64_t>(handle_));
    set_quality->set_quality(quality);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_set_remote_track_publication_quality()) {
        throw std::runtime_error("Invalid response for SetRemoteTrackPublicationQuality");
    }
}

void RemoteTrackPublication::SetVideoDimension(uint32_t width, uint32_t height)
{
    proto::FfiRequest req;
    auto* set_dimension = req.mutable_update_remote_track_publication_dimension();
    set_dimension->set_track_publication_handle(static_cast<uint64_t>(handle_));
    set_dimension->set_width(width);
    set_dimension->set_height(height);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_update_remote_track_publication_dimension()) {
        throw std::runtime_error("Invalid response for UpdateRemoteTrackPublicationDimension");
    }
}

void RemoteTrackPublication::SetEnabled(bool enabled)
{
    proto::FfiRequest req;
    auto* enable_req = req.mutable_enable_remote_track_publication();
    enable_req->set_track_publication_handle(static_cast<uint64_t>(handle_));
    enable_req->set_enabled(enabled);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_enable_remote_track_publication()) {
        throw std::runtime_error("Invalid response for EnableRemoteTrackPublication");
    }
}

void RemoteTrackPublication::SetTrack(std::shared_ptr<RemoteTrack> track)
{
    track_ = track;
}

} // namespace livekit
