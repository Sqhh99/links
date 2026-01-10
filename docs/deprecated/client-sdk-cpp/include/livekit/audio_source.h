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

#pragma once

#include "livekit/ffi_client.h"
#include "audio_frame.pb.h"
#include <memory>
#include <vector>

namespace livekit
{

// Forward declarations
class LocalTrack;

// Helper function to create audio source handle
static uintptr_t CreateAudioSourceHandle(uint32_t sample_rate, uint32_t num_channels)
{
    proto::FfiRequest req;
    auto* new_source = req.mutable_new_audio_source();
    new_source->set_type(proto::AUDIO_SOURCE_NATIVE);
    new_source->set_sample_rate(sample_rate);
    new_source->set_num_channels(num_channels);
    new_source->mutable_options()->set_echo_cancellation(false);
    new_source->mutable_options()->set_noise_suppression(false);
    new_source->mutable_options()->set_auto_gain_control(false);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_new_audio_source()) {
        throw std::runtime_error("Failed to create audio source");
    }

    return static_cast<uintptr_t>(resp.new_audio_source().source().handle().id());
}

// Audio source for capturing microphone
class AudioSource
{
public:
    AudioSource(uint32_t sample_rate = 48000, uint32_t num_channels = 1);
    ~AudioSource();

    // Capture an audio frame
    void CaptureFrame(const int16_t* audio_data, 
                     uint32_t samples_per_channel);

    // Get the FFI handle
    const FfiHandle& GetHandle() const { return handle_; }

    // Create a track from this source
    std::shared_ptr<LocalTrack> CreateTrack(const std::string& name);

    uint32_t GetSampleRate() const { return sample_rate_; }
    uint32_t GetNumChannels() const { return num_channels_; }

private:
    FfiHandle handle_;
    uint32_t sample_rate_;
    uint32_t num_channels_;
};

} // namespace livekit
