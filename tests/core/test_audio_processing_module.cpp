#define _USE_MATH_DEFINES
#include <cmath>
#include <gtest/gtest.h>
#include "core/audio_processing_module.h"
#include <vector>

// =============================================================================
// AudioProcessingModule Unit Tests
// =============================================================================

class AudioProcessingModuleTest : public ::testing::Test {
protected:
    AudioProcessingModule apm;
    
    // Generate test audio data (sine wave)
    std::vector<int16_t> generateSineWave(int samples, int sampleRate, int frequency) {
        std::vector<int16_t> data(samples);
        for (int i = 0; i < samples; ++i) {
            double t = static_cast<double>(i) / sampleRate;
            data[i] = static_cast<int16_t>(16000.0 * std::sin(2.0 * M_PI * frequency * t));
        }
        return data;
    }
    
    // Generate silence
    std::vector<int16_t> generateSilence(int samples) {
        return std::vector<int16_t>(samples, 0);
    }
    
    // Generate white noise
    std::vector<int16_t> generateNoise(int samples, int amplitude = 5000) {
        std::vector<int16_t> data(samples);
        for (int i = 0; i < samples; ++i) {
            data[i] = static_cast<int16_t>((rand() % (2 * amplitude)) - amplitude);
        }
        return data;
    }
};

// Test: Default construction
TEST_F(AudioProcessingModuleTest, DefaultConstruction) {
    EXPECT_FALSE(apm.isInitialized());
    EXPECT_TRUE(apm.isEchoCancellationEnabled());
    EXPECT_TRUE(apm.isNoiseSuppressionEnabled());
    EXPECT_TRUE(apm.isAutoGainControlEnabled());
}

// Test: Initialization
TEST_F(AudioProcessingModuleTest, Initialize) {
    EXPECT_TRUE(apm.initialize());
    EXPECT_TRUE(apm.isInitialized());
    
    // Second initialization should also succeed (idempotent)
    EXPECT_TRUE(apm.initialize());
}

// Test: Configuration setters
TEST_F(AudioProcessingModuleTest, ConfigurationSetters) {
    apm.initialize();
    
    apm.setEchoCancellationEnabled(false);
    EXPECT_FALSE(apm.isEchoCancellationEnabled());
    
    apm.setNoiseSuppressionEnabled(false);
    EXPECT_FALSE(apm.isNoiseSuppressionEnabled());
    
    apm.setAutoGainControlEnabled(false);
    EXPECT_FALSE(apm.isAutoGainControlEnabled());
    
    // Re-enable
    apm.setEchoCancellationEnabled(true);
    apm.setNoiseSuppressionEnabled(true);
    apm.setAutoGainControlEnabled(true);
    
    EXPECT_TRUE(apm.isEchoCancellationEnabled());
    EXPECT_TRUE(apm.isNoiseSuppressionEnabled());
    EXPECT_TRUE(apm.isAutoGainControlEnabled());
}

// Test: Process frame with valid data
TEST_F(AudioProcessingModuleTest, ProcessFrame_ValidData) {
    ASSERT_TRUE(apm.initialize());
    
    // Generate 10ms of audio at 48kHz (480 samples)
    auto audio = generateSineWave(480, 48000, 440);
    
    // Process should succeed
    EXPECT_TRUE(apm.processFrame(audio.data(), 480, 48000, 1));
}

// Test: Process frame with silence
TEST_F(AudioProcessingModuleTest, ProcessFrame_Silence) {
    ASSERT_TRUE(apm.initialize());
    
    auto silence = generateSilence(480);
    EXPECT_TRUE(apm.processFrame(silence.data(), 480, 48000, 1));
    
    // Verify output is still silence (or very close to it)
    int64_t sumAbs = 0;
    for (int16_t sample : silence) {
        sumAbs += std::abs(sample);
    }
    // Average should be very low (AGC might add slight noise)
    EXPECT_LT(sumAbs / 480, 100);
}

// Test: Process frame without initialization
TEST_F(AudioProcessingModuleTest, ProcessFrame_NotInitialized) {
    auto audio = generateSineWave(480, 48000, 440);
    EXPECT_FALSE(apm.processFrame(audio.data(), 480, 48000, 1));
}

// Test: Process frame with null data
TEST_F(AudioProcessingModuleTest, ProcessFrame_NullData) {
    ASSERT_TRUE(apm.initialize());
    EXPECT_FALSE(apm.processFrame(nullptr, 480, 48000, 1));
}

// Test: Process frame with zero samples
TEST_F(AudioProcessingModuleTest, ProcessFrame_ZeroSamples) {
    ASSERT_TRUE(apm.initialize());
    auto audio = generateSineWave(480, 48000, 440);
    EXPECT_FALSE(apm.processFrame(audio.data(), 0, 48000, 1));
}

// Test: Process multiple frames
TEST_F(AudioProcessingModuleTest, ProcessMultipleFrames) {
    ASSERT_TRUE(apm.initialize());
    
    // Process 100 frames (1 second at 48kHz)
    for (int i = 0; i < 100; ++i) {
        auto audio = generateSineWave(480, 48000, 440);
        EXPECT_TRUE(apm.processFrame(audio.data(), 480, 48000, 1));
    }
}

// Test: Audio energy is preserved (approximately)
TEST_F(AudioProcessingModuleTest, AudioEnergyPreservation) {
    ASSERT_TRUE(apm.initialize());
    
    // Disable ALL processing to test energy preservation
    // APM will still process but should pass through with minimal changes
    apm.setAutoGainControlEnabled(false);
    apm.setEchoCancellationEnabled(false);
    apm.setNoiseSuppressionEnabled(false);
    
    auto audio = generateSineWave(480, 48000, 440);
    
    // Calculate input energy
    double inputEnergy = 0;
    for (int16_t sample : audio) {
        inputEnergy += static_cast<double>(sample) * sample;
    }
    
    // Process
    EXPECT_TRUE(apm.processFrame(audio.data(), 480, 48000, 1));
    
    // Calculate output energy
    double outputEnergy = 0;
    for (int16_t sample : audio) {
        outputEnergy += static_cast<double>(sample) * sample;
    }
    
    // With all processing disabled, output should be very similar to input
    // Allow some tolerance for high-pass filter and other minimal processing
    if (inputEnergy > 0) {
        double ratio = outputEnergy / inputEnergy;
        EXPECT_GT(ratio, 0.1);  // At least 10% energy preserved
        EXPECT_LT(ratio, 10.0); // No more than 10x amplification
    }
}

// Test: Move semantics
TEST_F(AudioProcessingModuleTest, MoveSemantics) {
    ASSERT_TRUE(apm.initialize());
    apm.setEchoCancellationEnabled(false);
    
    // Move construct
    AudioProcessingModule apm2 = std::move(apm);
    EXPECT_TRUE(apm2.isInitialized());
    EXPECT_FALSE(apm2.isEchoCancellationEnabled());
    
    // Move assign
    AudioProcessingModule apm3;
    apm3 = std::move(apm2);
    EXPECT_TRUE(apm3.isInitialized());
}
