/*
 * Copyright 2023 LiveKit
 *
 * Licensed under the Apache License, Version 2.0 (the “License”);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an “AS IS” BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIVEKIT_FFI_CLIENT_H
#define LIVEKIT_FFI_CLIENT_H

#include <iostream>
#include <memory>
#include <functional>
#include <mutex>
#include <unordered_map>

#include "ffi.pb.h"
#include "livekit_ffi.h"

namespace livekit
{
    using FfiCallbackFn = void(*)(const uint8_t*, size_t);
    extern "C" void livekit_ffi_initialize(FfiCallbackFn cb,
                            bool capture_logs,
                            const char* sdk,
                            const char* sdk_version);

    extern "C" void LivekitFfiCallback(const uint8_t *buf, size_t len);


    // The FfiClient is used to communicate with the FFI interface of the Rust SDK
    // We use the generated protocol messages to facilitate the communication
    class FfiClient
    {
    public:
        using ListenerId = int;
        using Listener = std::function<void(const proto::FfiEvent&)>;

        FfiClient(const FfiClient&) = delete;
        FfiClient& operator=(const FfiClient&) = delete;

        static FfiClient& getInstance() {
            static FfiClient instance;
            return instance;
        }
 
        ListenerId AddListener(const Listener& listener);
        void RemoveListener(ListenerId id);

        proto::FfiResponse SendRequest(const proto::FfiRequest& request)const;

    private:
        std::unordered_map<ListenerId, Listener> listeners_;
        ListenerId nextListenerId = 1;
        mutable std::mutex lock_;

        FfiClient();
        ~FfiClient() = default;

        void PushEvent(const proto::FfiEvent& event) const;
        friend void LivekitFfiCallback(const uint8_t *buf, size_t len);
    };

    struct FfiHandle {
        uintptr_t handle;

        FfiHandle(uintptr_t handle);
        ~FfiHandle();
        
        // Delete copy constructor and copy assignment
        FfiHandle(const FfiHandle&) = delete;
        FfiHandle& operator=(const FfiHandle&) = delete;
        
        // Add move constructor and move assignment
        FfiHandle(FfiHandle&& other) noexcept : handle(other.handle) {
            other.handle = INVALID_HANDLE;  // Prevent double-free
        }
        
        FfiHandle& operator=(FfiHandle&& other) noexcept {
            if (this != &other) {
                // Clean up existing handle
                if (handle != INVALID_HANDLE) {
                    livekit_ffi_drop_handle(handle);
                }
                // Move handle from other
                handle = other.handle;
                other.handle = INVALID_HANDLE;  // Prevent double-free
            }
            return *this;
        }
        
        // Comparison operators
        bool operator==(uintptr_t other) const { return handle == other; }
        bool operator!=(uintptr_t other) const { return handle != other; }
        
        // Conversion operator to uint64_t
        explicit operator uint64_t() const { return static_cast<uint64_t>(handle); }
    };
}

#endif /* LIVEKIT_FFI_CLIENT_H */
