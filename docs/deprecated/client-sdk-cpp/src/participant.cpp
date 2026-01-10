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

#include "livekit/participant.h"
#include "livekit/track.h"
#include "livekit/ffi_client.h"
#include "room.pb.h"
#include <iostream>

namespace livekit
{

// ============================================================================
// Participant Base Class
// ============================================================================

Participant::Participant(const proto::ParticipantInfo& info, FfiHandle handle)
    : info_(info), handle_(std::move(handle))
{
    // Copy attributes from protobuf map
    for (const auto& [key, value] : info_.attributes()) {
        attributes_[key] = value;
    }
}

std::string Participant::GetAttribute(const std::string& key) const
{
    auto it = attributes_.find(key);
    return it != attributes_.end() ? it->second : "";
}

std::vector<std::shared_ptr<TrackPublication>> Participant::GetTracks() const
{
    std::vector<std::shared_ptr<TrackPublication>> tracks;
    tracks.reserve(track_publications_.size());
    for (const auto& [_, pub] : track_publications_) {
        tracks.push_back(pub);
    }
    return tracks;
}

std::shared_ptr<TrackPublication> Participant::GetTrack(const std::string& sid) const
{
    auto it = track_publications_.find(sid);
    return it != track_publications_.end() ? it->second : nullptr;
}

// ============================================================================
// LocalParticipant
// ============================================================================

LocalParticipant::LocalParticipant(const proto::ParticipantInfo& info, FfiHandle handle)
    : Participant(info, std::move(handle))
{
}

std::shared_ptr<LocalTrackPublication> LocalParticipant::PublishTrack(
    std::shared_ptr<LocalTrack> track,
    const proto::TrackPublishOptions& options)
{
    proto::FfiRequest req;
    auto* publish = req.mutable_publish_track();
    publish->set_local_participant_handle(static_cast<uint64_t>(handle_));
    publish->set_track_handle(static_cast<uint64_t>(track->GetHandle()));
    *publish->mutable_options() = options;

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_publish_track()) {
        throw std::runtime_error("Invalid response for PublishTrack");
    }

    // The actual publication will be received via PublishTrackCallback event
    // For now, return nullptr and let the Room handle the callback
    return nullptr;
}

void LocalParticipant::UnpublishTrack(const std::string& track_sid)
{
    proto::FfiRequest req;
    auto* unpublish = req.mutable_unpublish_track();
    unpublish->set_local_participant_handle(static_cast<uint64_t>(handle_));
    unpublish->set_track_sid(track_sid);
    unpublish->set_stop_on_unpublish(true);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_unpublish_track()) {
        throw std::runtime_error("Invalid response for UnpublishTrack");
    }
}

void LocalParticipant::SetMetadata(const std::string& metadata)
{
    proto::FfiRequest req;
    auto* set_metadata = req.mutable_set_local_metadata();
    set_metadata->set_local_participant_handle(static_cast<uint64_t>(handle_));
    set_metadata->set_metadata(metadata);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_set_local_metadata()) {
        throw std::runtime_error("Invalid response for SetLocalMetadata");
    }

    // Update local info
    info_.set_metadata(metadata);
}

void LocalParticipant::SetName(const std::string& name)
{
    proto::FfiRequest req;
    auto* set_name = req.mutable_set_local_name();
    set_name->set_local_participant_handle(static_cast<uint64_t>(handle_));
    set_name->set_name(name);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_set_local_name()) {
        throw std::runtime_error("Invalid response for SetLocalName");
    }

    // Update local info
    info_.set_name(name);
}

void LocalParticipant::SetAttributes(const std::map<std::string, std::string>& attributes)
{
    proto::FfiRequest req;
    auto* set_attrs = req.mutable_set_local_attributes();
    set_attrs->set_local_participant_handle(static_cast<uint64_t>(handle_));
    
    for (const auto& [key, value] : attributes) {
        auto* entry = set_attrs->add_attributes();
        entry->set_key(key);
        entry->set_value(value);
    }

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_set_local_attributes()) {
        throw std::runtime_error("Invalid response for SetLocalAttributes");
    }

    // Update local attributes
    attributes_ = attributes;
}

void LocalParticipant::PublishData(
    const std::vector<uint8_t>& data,
    bool reliable,
    const std::string& topic,
    const std::vector<std::string>& destination_identities)
{
    proto::FfiRequest req;
    auto* publish_data = req.mutable_publish_data();
    publish_data->set_local_participant_handle(static_cast<uint64_t>(handle_));
    
    // Note: FFI expects data_ptr and data_len, but we need to keep data alive
    // This is a simplified implementation - in production, you'd need proper memory management
    publish_data->set_data_ptr(reinterpret_cast<uint64_t>(data.data()));
    publish_data->set_data_len(data.size());
    publish_data->set_reliable(reliable);
    
    if (!topic.empty()) {
        publish_data->set_topic(topic);
    }
    
    for (const auto& identity : destination_identities) {
        publish_data->add_destination_identities(identity);
    }

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_publish_data()) {
        throw std::runtime_error("Invalid response for PublishData");
    }
}

void LocalParticipant::UpdateInfo(const proto::ParticipantInfo& info)
{
    info_ = info;
    
    // Update attributes
    attributes_.clear();
    for (const auto& [key, value] : info_.attributes()) {
        attributes_[key] = value;
    }
}

// ============================================================================
// RemoteParticipant
// ============================================================================

RemoteParticipant::RemoteParticipant(const proto::ParticipantInfo& info, FfiHandle handle)
    : Participant(info, std::move(handle)),
      connection_quality_(proto::QUALITY_EXCELLENT)
{
}

void RemoteParticipant::UpdateInfo(const proto::ParticipantInfo& info)
{
    info_ = info;
    
    // Update attributes
    attributes_.clear();
    for (const auto& [key, value] : info_.attributes()) {
        attributes_[key] = value;
    }
}

void RemoteParticipant::SetConnectionQuality(proto::ConnectionQuality quality)
{
    connection_quality_ = quality;
}

} // namespace livekit
