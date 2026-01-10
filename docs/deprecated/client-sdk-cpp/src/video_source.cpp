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

#include "livekit/video_source.h"
#include "livekit/track.h"
#include "video_frame.pb.h"
#include <chrono>
#include <iostream>

namespace livekit
{

VideoSource::VideoSource(uint32_t width, uint32_t height)
    : width_(width), height_(height), handle_(CreateVideoSourceHandle(width, height))
{
    std::cout << "VideoSource created with handle: " << handle_.handle << std::endl;
}

VideoSource::~VideoSource()
{
    // Dispose happens automatically in FfiHandle destructor
    std::cout << "VideoSource destroyed: " << handle_.handle << std::endl;
}

void VideoSource::CaptureFrame(const uint8_t* rgba_data, 
                              uint32_t width, 
                              uint32_t height,
                              int64_t timestamp_us)
{
    if (timestamp_us == 0) {
        auto now = std::chrono::system_clock::now();
        timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
    }

    proto::FfiRequest req;
    auto* capture = req.mutable_capture_video_frame();
    capture->set_source_handle(static_cast<uint64_t>(handle_));
    capture->set_timestamp_us(timestamp_us);
    capture->set_rotation(proto::VIDEO_ROTATION_0);

    // Set up buffer info
    auto* buffer = capture->mutable_buffer();
    buffer->set_type(proto::RGBA);
    buffer->set_width(width);
    buffer->set_height(height);
    buffer->set_data_ptr(reinterpret_cast<uint64_t>(rgba_data));
    
    // For RGBA, stride is width * 4 bytes
    buffer->set_stride(width * 4);

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    if (!resp.has_capture_video_frame()) {
        std::cerr << "Failed to capture video frame" << std::endl;
    }
}

std::shared_ptr<LocalTrack> VideoSource::CreateTrack(const std::string& name)
{
    std::cout << "VideoSource::CreateTrack called with name: " << name << std::endl;
    std::cout << "VideoSource handle: " << handle_.handle << std::endl;
    
    proto::FfiRequest req;
    auto* create_track = req.mutable_create_video_track();
    create_track->set_name(name);
    create_track->set_source_handle(static_cast<uint64_t>(handle_.handle));
    
    std::cout << "Sending CreateVideoTrack request with source_handle: " 
              << create_track->source_handle() << std::endl;

    proto::FfiResponse resp = FfiClient::getInstance().SendRequest(req);
    
    std::cout << "Received response, checking for create_video_track..." << std::endl;
    
    if (!resp.has_create_video_track()) {
        std::cout << "ERROR: Response does not have create_video_track!" << std::endl;
        throw std::runtime_error("Failed to create video track");
    }

    const auto& track_info = resp.create_video_track().track();
    FfiHandle track_handle(static_cast<uintptr_t>(track_info.handle().id()));

    std::cout << "VideoTrack created with handle: " << track_handle.handle << std::endl;

    return std::make_shared<LocalTrack>(track_info.info(), std::move(track_handle));
}

} // namespace livekit
