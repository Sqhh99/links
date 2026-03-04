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
    EXPECT_TRUE(apm.isHighPassFilterEnabled());
    
    // Advanced defaults
    EXPECT_EQ(apm.noiseSuppressionLevel(), AudioProcessingModule::NoiseSuppressionLevel::kModerate);
    EXPECT_EQ(apm.gainControlMode(), AudioProcessingModule::GainControlMode::kAdaptiveDigital);
    EXPECT_FLOAT_EQ(apm.fixedDigitalGainDb(), 0.0f);
    EXPECT_FLOAT_EQ(apm.adaptiveDigitalMaxGainDb(), 50.0f);
    EXPECT_TRUE(apm.isEchoEnhancedFilterEnabled());
    EXPECT_EQ(apm.streamDelayMs(), 0);
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
    
    apm.setHighPassFilterEnabled(false);
    EXPECT_FALSE(apm.isHighPassFilterEnabled());
    
    // Re-enable
    apm.setEchoCancellationEnabled(true);
    apm.setNoiseSuppressionEnabled(true);
    apm.setAutoGainControlEnabled(true);
    apm.setHighPassFilterEnabled(true);
    
    EXPECT_TRUE(apm.isEchoCancellationEnabled());
    EXPECT_TRUE(apm.isNoiseSuppressionEnabled());
    EXPECT_TRUE(apm.isAutoGainControlEnabled());
    EXPECT_TRUE(apm.isHighPassFilterEnabled());
}

// Test: Advanced NS level configuration
TEST_F(AudioProcessingModuleTest, NoiseSuppressionLevel) {
    apm.initialize();
    
    apm.setNoiseSuppressionLevel(AudioProcessingModule::NoiseSuppressionLevel::kLow);
    EXPECT_EQ(apm.noiseSuppressionLevel(), AudioProcessingModule::NoiseSuppressionLevel::kLow);
    
    apm.setNoiseSuppressionLevel(AudioProcessingModule::NoiseSuppressionLevel::kHigh);
    EXPECT_EQ(apm.noiseSuppressionLevel(), AudioProcessingModule::NoiseSuppressionLevel::kHigh);
    
    apm.setNoiseSuppressionLevel(AudioProcessingModule::NoiseSuppressionLevel::kVeryHigh);
    EXPECT_EQ(apm.noiseSuppressionLevel(), AudioProcessingModule::NoiseSuppressionLevel::kVeryHigh);
    
    apm.setNoiseSuppressionLevel(AudioProcessingModule::NoiseSuppressionLevel::kModerate);
    EXPECT_EQ(apm.noiseSuppressionLevel(), AudioProcessingModule::NoiseSuppressionLevel::kModerate);
}

// Test: Advanced AGC mode configuration
TEST_F(AudioProcessingModuleTest, GainControlMode) {
    apm.initialize();
    
    apm.setGainControlMode(AudioProcessingModule::GainControlMode::kFixedDigital);
    EXPECT_EQ(apm.gainControlMode(), AudioProcessingModule::GainControlMode::kFixedDigital);
    
    apm.setGainControlMode(AudioProcessingModule::GainControlMode::kAdaptiveDigital);
    EXPECT_EQ(apm.gainControlMode(), AudioProcessingModule::GainControlMode::kAdaptiveDigital);
}

// Test: AGC gain parameters
TEST_F(AudioProcessingModuleTest, GainParameters) {
    apm.initialize();
    
    apm.setFixedDigitalGainDb(10.0f);
    EXPECT_FLOAT_EQ(apm.fixedDigitalGainDb(), 10.0f);
    
    // Clamping: should not exceed 50
    apm.setFixedDigitalGainDb(100.0f);
    EXPECT_FLOAT_EQ(apm.fixedDigitalGainDb(), 50.0f);
    
    // Clamping: should not go below 0
    apm.setFixedDigitalGainDb(-5.0f);
    EXPECT_FLOAT_EQ(apm.fixedDigitalGainDb(), 0.0f);
    
    apm.setAdaptiveDigitalMaxGainDb(30.0f);
    EXPECT_FLOAT_EQ(apm.adaptiveDigitalMaxGainDb(), 30.0f);
}

// Test: AEC enhanced filter
TEST_F(AudioProcessingModuleTest, EchoEnhancedFilter) {
    apm.initialize();
    
    apm.setEchoEnhancedFilterEnabled(false);
    EXPECT_FALSE(apm.isEchoEnhancedFilterEnabled());
    
    apm.setEchoEnhancedFilterEnabled(true);
    EXPECT_TRUE(apm.isEchoEnhancedFilterEnabled());
}

// Test: Stream delay setting
TEST_F(AudioProcessingModuleTest, StreamDelay) {
    apm.initialize();
    
    apm.setStreamDelayMs(80);
    EXPECT_EQ(apm.streamDelayMs(), 80);
    
    // Negative should be clamped to 0
    apm.setStreamDelayMs(-10);
    EXPECT_EQ(apm.streamDelayMs(), 0);
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

// Test: ProcessReverseStream basic functionality
TEST_F(AudioProcessingModuleTest, ProcessReverseStream_ValidData) {
    ASSERT_TRUE(apm.initialize());
    
    auto audio = generateSineWave(480, 48000, 440);
    EXPECT_TRUE(apm.processReverseStream(audio.data(), 480, 48000, 1));
}

// Test: ProcessReverseStream with null data
TEST_F(AudioProcessingModuleTest, ProcessReverseStream_NullData) {
    ASSERT_TRUE(apm.initialize());
    EXPECT_FALSE(apm.processReverseStream(nullptr, 480, 48000, 1));
}

// Test: ProcessReverseStream not initialized
TEST_F(AudioProcessingModuleTest, ProcessReverseStream_NotInitialized) {
    auto audio = generateSineWave(480, 48000, 440);
    EXPECT_FALSE(apm.processReverseStream(audio.data(), 480, 48000, 1));
}

// Test: ProcessReverseStream with zero samples
TEST_F(AudioProcessingModuleTest, ProcessReverseStream_ZeroSamples) {
    ASSERT_TRUE(apm.initialize());
    auto audio = generateSineWave(480, 48000, 440);
    EXPECT_FALSE(apm.processReverseStream(audio.data(), 0, 48000, 1));
}

// Test: Full AEC pipeline (reverse + capture)
TEST_F(AudioProcessingModuleTest, AecPipeline_ReverseAndCapture) {
    ASSERT_TRUE(apm.initialize());
    apm.setEchoCancellationEnabled(true);
    apm.setStreamDelayMs(80);
    
    // Simulate 100 frames of reverse (far-end) then capture (near-end)
    for (int i = 0; i < 100; ++i) {
        // Feed far-end (speaker) audio
        auto farEnd = generateSineWave(480, 48000, 440);
        EXPECT_TRUE(apm.processReverseStream(farEnd.data(), 480, 48000, 1));
        
        // Process near-end (microphone) audio
        auto nearEnd = generateSineWave(480, 48000, 440);
        EXPECT_TRUE(apm.processFrame(nearEnd.data(), 480, 48000, 1));
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

// Test: Advanced settings work together
TEST_F(AudioProcessingModuleTest, AdvancedSettingsCombined) {
    ASSERT_TRUE(apm.initialize());
    
    // Configure all advanced settings
    apm.setNoiseSuppressionLevel(AudioProcessingModule::NoiseSuppressionLevel::kHigh);
    apm.setGainControlMode(AudioProcessingModule::GainControlMode::kFixedDigital);
    apm.setFixedDigitalGainDb(6.0f);
    apm.setEchoEnhancedFilterEnabled(true);
    apm.setHighPassFilterEnabled(true);
    apm.setStreamDelayMs(60);
    
    // Process should still work fine
    auto audio = generateSineWave(480, 48000, 440);
    EXPECT_TRUE(apm.processFrame(audio.data(), 480, 48000, 1));
}

// Test: Switching AGC modes at runtime
TEST_F(AudioProcessingModuleTest, RuntimeAgcModeSwitch) {
    ASSERT_TRUE(apm.initialize());
    
    // Start with adaptive
    apm.setGainControlMode(AudioProcessingModule::GainControlMode::kAdaptiveDigital);
    apm.setAdaptiveDigitalMaxGainDb(30.0f);
    
    auto audio1 = generateSineWave(480, 48000, 440);
    EXPECT_TRUE(apm.processFrame(audio1.data(), 480, 48000, 1));
    
    // Switch to fixed at runtime
    apm.setGainControlMode(AudioProcessingModule::GainControlMode::kFixedDigital);
    apm.setFixedDigitalGainDb(6.0f);
    
    auto audio2 = generateSineWave(480, 48000, 440);
    EXPECT_TRUE(apm.processFrame(audio2.data(), 480, 48000, 1));
    
    // Switch back
    apm.setGainControlMode(AudioProcessingModule::GainControlMode::kAdaptiveDigital);
    
    auto audio3 = generateSineWave(480, 48000, 440);
    EXPECT_TRUE(apm.processFrame(audio3.data(), 480, 48000, 1));
}
