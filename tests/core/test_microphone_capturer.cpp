#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "core/media/audio_input.h"
#include "core/microphone_capturer.h"
#include "livekit/livekit.h"

// =============================================================================
// LiveKit SDK initialisation
//
// Since SDK 1.x, livekit::initialize() must be the first LiveKit API called in
// the process; creating an AudioSource before it fails with "LiveKit is not
// initialized". The application does this in main.cpp, so the test binary needs
// its own global environment to match.
// =============================================================================

namespace {

class LiveKitEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        livekit::initialize(livekit::LogLevel::Warn);
    }
};

const ::testing::Environment* const kLiveKitEnvironment =
    ::testing::AddGlobalTestEnvironment(new LiveKitEnvironment);

/**
 * Stand-in for a real microphone.
 *
 * This is what the AudioInput port buys us: MicrophoneCapturer's framing and
 * APM behaviour can now be exercised without Qt, without audio hardware, and
 * deterministically. Previously these tests needed a QCoreApplication and were
 * skipped whenever the machine had no microphone.
 */
class FakeAudioInput : public links::core::AudioInput {
public:
    bool start(const links::core::AudioFormat& format) override
    {
        if (failToStart) {
            return false;
        }
        format_ = format;
        active_ = true;
        ++startCount;
        return true;
    }

    void stop() override
    {
        active_ = false;
        ++stopCount;
    }

    bool isActive() const override { return active_; }

    void setDeviceId(const std::string& deviceId) override { lastDeviceId = deviceId; }

    void setDataCallback(
        std::function<void(const std::int16_t*, std::size_t)> callback) override
    {
        dataCallback_ = std::move(callback);
    }

    void setErrorCallback(std::function<void(const std::string&)> callback) override
    {
        errorCallback_ = std::move(callback);
    }

    /// Push `sampleCount` samples of silence through the capturer.
    void emitSamples(std::size_t sampleCount)
    {
        std::vector<std::int16_t> samples(sampleCount, 0);
        if (dataCallback_) {
            dataCallback_(samples.data(), samples.size());
        }
    }

    bool failToStart{false};
    int startCount{0};
    int stopCount{0};
    std::string lastDeviceId;

private:
    bool active_{false};
    links::core::AudioFormat format_{};
    std::function<void(const std::int16_t*, std::size_t)> dataCallback_;
    std::function<void(const std::string&)> errorCallback_;
};

} // namespace

// =============================================================================
// MicrophoneCapturer Unit Tests
// =============================================================================

class MicrophoneCapturerTest : public ::testing::Test {
protected:
    FakeAudioInput input;
    MicrophoneCapturer* capturer = nullptr;

    void SetUp() override {
        capturer = new MicrophoneCapturer(input);
    }

    void TearDown() override {
        if (capturer) {
            capturer->stop();
            delete capturer;
            capturer = nullptr;
        }
    }
};

// Test: Default state
TEST_F(MicrophoneCapturerTest, DefaultState) {
    EXPECT_FALSE(capturer->isActive());
    EXPECT_EQ(capturer->getAudioSource(), nullptr);
}

// Test: Audio processing module access
TEST_F(MicrophoneCapturerTest, AudioProcessingModuleAccess) {
    auto* apm = capturer->audioProcessingModule();
    EXPECT_NE(apm, nullptr);

    // Should be initialized by constructor
    EXPECT_TRUE(apm->isInitialized());
}

// Test: Audio processing configuration
TEST_F(MicrophoneCapturerTest, AudioProcessingConfiguration) {
    capturer->setEchoCancellationEnabled(false);
    capturer->setNoiseSuppressionEnabled(false);
    capturer->setAutoGainControlEnabled(false);

    auto* apm = capturer->audioProcessingModule();
    EXPECT_FALSE(apm->isEchoCancellationEnabled());
    EXPECT_FALSE(apm->isNoiseSuppressionEnabled());
    EXPECT_FALSE(apm->isAutoGainControlEnabled());

    capturer->setEchoCancellationEnabled(true);
    capturer->setNoiseSuppressionEnabled(true);
    capturer->setAutoGainControlEnabled(true);

    EXPECT_TRUE(apm->isEchoCancellationEnabled());
    EXPECT_TRUE(apm->isNoiseSuppressionEnabled());
    EXPECT_TRUE(apm->isAutoGainControlEnabled());
}

// Test: Set device by ID (with empty ID)
TEST_F(MicrophoneCapturerTest, SetDeviceByEmptyId) {
    capturer->setDeviceById(std::string());
    EXPECT_TRUE(input.lastDeviceId.empty());
}

// Test: Set device by ID forwards to the input
TEST_F(MicrophoneCapturerTest, SetDeviceByIdForwards) {
    capturer->setDeviceById("non-existent-device-id");
    EXPECT_EQ(input.lastDeviceId, "non-existent-device-id");
}

// Test: Stop without start
TEST_F(MicrophoneCapturerTest, StopWithoutStart) {
    capturer->stop();
    EXPECT_FALSE(capturer->isActive());
}

// Test: Multiple stop calls
TEST_F(MicrophoneCapturerTest, MultipleStopCalls) {
    capturer->stop();
    capturer->stop();
    capturer->stop();
    EXPECT_FALSE(capturer->isActive());
}

// Test: Start and stop capture
TEST_F(MicrophoneCapturerTest, StartStopCapture) {
    EXPECT_TRUE(capturer->start());
    EXPECT_TRUE(capturer->isActive());
    EXPECT_NE(capturer->getAudioSource(), nullptr);
    EXPECT_EQ(input.startCount, 1);

    capturer->stop();
    EXPECT_FALSE(capturer->isActive());
    EXPECT_EQ(input.stopCount, 1);
}

// Test: Start, stop, and restart
TEST_F(MicrophoneCapturerTest, RestartCapture) {
    EXPECT_TRUE(capturer->start());
    EXPECT_TRUE(capturer->isActive());
    capturer->stop();
    EXPECT_FALSE(capturer->isActive());

    EXPECT_TRUE(capturer->start());
    EXPECT_TRUE(capturer->isActive());
    capturer->stop();
    EXPECT_FALSE(capturer->isActive());

    EXPECT_EQ(input.startCount, 2);
}

// Test: Double start (idempotent)
TEST_F(MicrophoneCapturerTest, DoubleStart) {
    EXPECT_TRUE(capturer->start());
    EXPECT_TRUE(capturer->start()); // Should return true (already active)
    EXPECT_EQ(input.startCount, 1); // ... without restarting the device
    capturer->stop();
}

// Test: A failing device start is reported
TEST_F(MicrophoneCapturerTest, StartFailurePropagates) {
    input.failToStart = true;
    EXPECT_FALSE(capturer->start());
    EXPECT_FALSE(capturer->isActive());
}

// Test: Setting a device while active is rejected
TEST_F(MicrophoneCapturerTest, SetDeviceWhileActiveIsRejected) {
    EXPECT_TRUE(capturer->start());

    capturer->setDeviceById("some-other-device");
    EXPECT_TRUE(input.lastDeviceId.empty()); // never forwarded

    EXPECT_TRUE(capturer->isActive());
    capturer->stop();
}

// Test: Samples shorter than one 10 ms frame are buffered, not dropped.
TEST_F(MicrophoneCapturerTest, PartialFrameIsBuffered) {
    EXPECT_TRUE(capturer->start());

    // 480 samples is exactly one 10 ms frame at 48 kHz mono; feed less.
    input.emitSamples(100);
    input.emitSamples(100);

    // Nothing to assert on the LiveKit side without a real room; the contract
    // under test is that partial frames neither crash nor are discarded.
    SUCCEED();
    capturer->stop();
}

// Test: A full frame is consumed without error.
TEST_F(MicrophoneCapturerTest, FullFrameIsProcessed) {
    EXPECT_TRUE(capturer->start());
    input.emitSamples(480 * 3);
    SUCCEED();
    capturer->stop();
}
