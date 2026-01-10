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

#include "livekit/audio_source.h"
#include "livekit/track.h"
#include "audio_frame.pb.h"
#include <iostream>

namespace livekit
{

AudioSource::AudioSource(uint32_t sample_rate, uint32_t num_channels)
    : sample_rate_(sample_rate), num_channels_(num_channels), handle_(CreateAudioSourceHandle(sample_rate, num_channels))
{
    std::cout << "AudioSource created with handle: " << handle_.handle
              << " (rate: " << sample_rate << ", channels: " << num_channels << ")" << std::endl;
}

AudioSource::~AudioSource()
{
    std::cout << "AudioSource destroyed: " << handle_.handle << std::endl;
}

void AudioSource::CaptureFrame(const int16_t* audio_data, 
                              uint32_t samples_per_channel)
{
    proto::FfiRequest req;
    auto* capture = req.mutable_capture_audio_frame();
    capture->set_source_handle(static_cast<uint64_t>(handle_));

    // Set up buffer info
    auto* buffer = capture->mutable_buffer();
    buffer->set_data_ptr(reinterpret_cast<uint64_t>(audio_data));
    buffer->set_num_channels(num_channels_);
    buffer->set_sample_rate(sample_rate_);
    buffer->set_samples_per_channel(samples_per_channel);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_capture_audio_frame()) {
        std::cerr << "Failed to capture audio frame" << std::endl;
    }
}

std::shared_ptr<LocalTrack> AudioSource::CreateTrack(const std::string& name)
{
    proto::FfiRequest req;
    auto* create_track = req.mutable_create_audio_track();
    create_track->set_name(name);
    create_track->set_source_handle(static_cast<uint64_t>(handle_.handle));

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_create_audio_track()) {
        throw std::runtime_error("Failed to create audio track");
    }

    const auto& track_info = resp.create_audio_track().track();
    FfiHandle track_handle(static_cast<uintptr_t>(track_info.handle().id()));

    std::cout << "AudioTrack created with handle: " << track_handle.handle << std::endl;

    return std::make_shared<LocalTrack>(track_info.info(), std::move(track_handle));
}

} // namespace livekit
