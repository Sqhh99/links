#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QThread>
#include <QAudioDevice>
#include <QMediaDevices>
#include "core/microphone_capturer.h"

// =============================================================================
// MicrophoneCapturer Unit Tests
// =============================================================================

class MicrophoneCapturerTest : public ::testing::Test {
protected:
    static QCoreApplication* app;
    MicrophoneCapturer* capturer = nullptr;
    
    static void SetUpTestSuite() {
        // Qt requires a QCoreApplication for audio devices
        if (!app) {
            int argc = 0;
            char** argv = nullptr;
            app = new QCoreApplication(argc, argv);
        }
    }
    
    void SetUp() override {
        capturer = new MicrophoneCapturer();
    }
    
    void TearDown() override {
        if (capturer) {
            capturer->stop();
            delete capturer;
            capturer = nullptr;
        }
    }
};

QCoreApplication* MicrophoneCapturerTest::app = nullptr;

// Test: Default state
TEST_F(MicrophoneCapturerTest, DefaultState) {
    EXPECT_FALSE(capturer->isActive());
    EXPECT_EQ(capturer->getAudioSource(), nullptr);
}

// Test: Available devices
TEST_F(MicrophoneCapturerTest, AvailableDevices) {
    auto devices = MicrophoneCapturer::availableDevices();
    // Should return a list (may be empty if no microphones)
    // Just verify it doesn't crash
    SUCCEED();
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
    // Set options
    capturer->setEchoCancellationEnabled(false);
    capturer->setNoiseSuppressionEnabled(false);
    capturer->setAutoGainControlEnabled(false);
    
    auto* apm = capturer->audioProcessingModule();
    EXPECT_FALSE(apm->isEchoCancellationEnabled());
    EXPECT_FALSE(apm->isNoiseSuppressionEnabled());
    EXPECT_FALSE(apm->isAutoGainControlEnabled());
    
    // Re-enable
    capturer->setEchoCancellationEnabled(true);
    capturer->setNoiseSuppressionEnabled(true);
    capturer->setAutoGainControlEnabled(true);
    
    EXPECT_TRUE(apm->isEchoCancellationEnabled());
    EXPECT_TRUE(apm->isNoiseSuppressionEnabled());
    EXPECT_TRUE(apm->isAutoGainControlEnabled());
}

// Test: Set device by ID (with empty ID)
TEST_F(MicrophoneCapturerTest, SetDeviceByEmptyId) {
    // Setting empty ID should not crash
    capturer->setDeviceById(QByteArray());
    SUCCEED();
}

// Test: Set device by non-existent ID
TEST_F(MicrophoneCapturerTest, SetDeviceByNonExistentId) {
    // Setting non-existent ID should not crash
    capturer->setDeviceById("non-existent-device-id");
    SUCCEED();
}

// Test: Stop without start
TEST_F(MicrophoneCapturerTest, StopWithoutStart) {
    // Stopping without starting should not crash
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

// =============================================================================
// Integration Tests (require actual microphone hardware)
// These tests are skipped if no microphone is available
// =============================================================================

class MicrophoneCapturerIntegrationTest : public MicrophoneCapturerTest {
protected:
    bool hasMicrophone() {
        return !QMediaDevices::audioInputs().isEmpty();
    }
};

// Test: Start and stop capture
TEST_F(MicrophoneCapturerIntegrationTest, StartStopCapture) {
    if (!hasMicrophone()) {
        GTEST_SKIP() << "No microphone available";
    }
    
    // Start
    EXPECT_TRUE(capturer->start());
    EXPECT_TRUE(capturer->isActive());
    EXPECT_NE(capturer->getAudioSource(), nullptr);
    
    // Let it capture for a short time
    QThread::msleep(100);
    
    // Stop
    capturer->stop();
    EXPECT_FALSE(capturer->isActive());
}

// Test: Start, stop, and restart
TEST_F(MicrophoneCapturerIntegrationTest, RestartCapture) {
    if (!hasMicrophone()) {
        GTEST_SKIP() << "No microphone available";
    }
    
    // First start/stop cycle
    EXPECT_TRUE(capturer->start());
    EXPECT_TRUE(capturer->isActive());
    QThread::msleep(50);
    capturer->stop();
    EXPECT_FALSE(capturer->isActive());
    
    // Second start/stop cycle
    EXPECT_TRUE(capturer->start());
    EXPECT_TRUE(capturer->isActive());
    QThread::msleep(50);
    capturer->stop();
    EXPECT_FALSE(capturer->isActive());
}

// Test: Double start (idempotent)
TEST_F(MicrophoneCapturerIntegrationTest, DoubleStart) {
    if (!hasMicrophone()) {
        GTEST_SKIP() << "No microphone available";
    }
    
    EXPECT_TRUE(capturer->start());
    EXPECT_TRUE(capturer->start()); // Should return true (already active)
    
    capturer->stop();
}

// Test: Set device while active (should warn)
TEST_F(MicrophoneCapturerIntegrationTest, SetDeviceWhileActive) {
    if (!hasMicrophone()) {
        GTEST_SKIP() << "No microphone available";
    }
    
    EXPECT_TRUE(capturer->start());
    
    // Setting device while active should be rejected (but not crash)
    auto devices = QMediaDevices::audioInputs();
    if (!devices.isEmpty()) {
        capturer->setDevice(devices.first());
    }
    
    // Should still be active
    EXPECT_TRUE(capturer->isActive());
    
    capturer->stop();
}

// Test: Switch devices
TEST_F(MicrophoneCapturerIntegrationTest, SwitchDevices) {
    auto devices = QMediaDevices::audioInputs();
    if (devices.size() < 2) {
        GTEST_SKIP() << "Need at least 2 microphones to test switching";
    }
    
    // Set first device
    capturer->setDevice(devices[0]);
    EXPECT_TRUE(capturer->start());
    QThread::msleep(50);
    capturer->stop();
    
    // Set second device
    capturer->setDevice(devices[1]);
    EXPECT_TRUE(capturer->start());
    QThread::msleep(50);
    capturer->stop();
}
