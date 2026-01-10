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
#include "video_frame.pb.h"
#include <memory>
#include <functional>

namespace livekit
{

// Forward declarations
class LocalTrack;

// Helper function to create video source handle
static uintptr_t CreateVideoSourceHandle(uint32_t width, uint32_t height)
{
    proto::FfiRequest req;
    auto* new_source = req.mutable_new_video_source();
    new_source->set_type(proto::VIDEO_SOURCE_NATIVE);
    
    auto* resolution = new_source->mutable_resolution();
    resolution->set_width(width);
    resolution->set_height(height);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_new_video_source()) {
        throw std::runtime_error("Failed to create video source");
    }

    return static_cast<uintptr_t>(resp.new_video_source().source().handle().id());
}

// Video source for capturing camera or screen
class VideoSource
{
public:
    VideoSource(uint32_t width, uint32_t height);
    ~VideoSource();

    // Capture a video frame
    void CaptureFrame(const uint8_t* rgba_data, 
                     uint32_t width, 
                     uint32_t height,
                     int64_t timestamp_us = 0);

    // Get the FFI handle
    const FfiHandle& GetHandle() const { return handle_; }

    // Create a track from this source
    std::shared_ptr<LocalTrack> CreateTrack(const std::string& name);

private:
    FfiHandle handle_;
    uint32_t width_;
    uint32_t height_;
};

} // namespace livekit
