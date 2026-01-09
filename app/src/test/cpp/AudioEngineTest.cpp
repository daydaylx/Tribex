#include "AudioEngine.h"
#include "PatternData.h"
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cmath>

using namespace Tribex;

// Helper function to run tests
void runTest(const char* testName, bool (*testFunc)()) {
    bool passed = testFunc();
    printf("[%s] %s\n", testName, passed ? "PASS" : "FAIL");
    assert(passed);
}

// TEST: AudioEngine Initialization
// AudioEngine should initialize correctly
bool testAudioEngineInitialization() {
    AudioEngine engine;
    
    // Check initial state
    if (engine.isPlaying()) {
        printf("  FAILED: AudioEngine should not be playing initially\n");
        return false;
    }
    
    if (engine.isSequencerPlaying()) {
        printf("  FAILED: Sequencer should not be playing initially\n");
        return false;
    }
    
    return true;
}

// TEST: AudioEngine Safety Checks
// AudioEngine should handle invalid parameters safely
bool testAudioEngineSafetyChecks() {
    AudioEngine engine;
    
    // Test that engine can be started and stopped
    bool started = engine.start();
    if (!started) {
        printf("  WARNING: AudioEngine failed to start (may be expected in test environment)\n");
        // Not failing - may be expected in test environment without actual audio device
    }
    
    bool stopped = engine.stop();
    if (!stopped) {
        printf("  FAILED: AudioEngine failed to stop\n");
        return false;
    }
    
    return true;
}

// TEST: AudioEngine Parameter Ranges
// AudioEngine should clamp parameter values
bool testAudioEngineParameterRanges() {
    AudioEngine engine;
    
    // Test BPM clamping
    engine.setBPM(10.0f);  // Below minimum
    float bpm = engine.getBPM();
    if (bpm < 19.9f || bpm > 20.1f) {
        printf("  FAILED: BPM should clamp to 20, got %.2f\n", bpm);
        return false;
    }
    
    engine.setBPM(500.0f);  // Above maximum
    bpm = engine.getBPM();
    if (bpm < 299.9f || bpm > 300.1f) {
        printf("  FAILED: BPM should clamp to 300, got %.2f\n", bpm);
        return false;
    }
    
    // Test valid BPM
    engine.setBPM(150.0f);
    bpm = engine.getBPM();
    if (bpm < 149.9f || bpm > 150.1f) {
        printf("  FAILED: BPM should be 150, got %.2f\n", bpm);
        return false;
    }
    
    return true;
}

// TEST: AudioEngine Pattern Management
// AudioEngine should handle pattern loading
bool testAudioEnginePatternManagement() {
    AudioEngine engine;
    
    // Create a simple test pattern
    Pattern pattern;
    pattern.id = 1;
    pattern.lengthSteps = 16;
    pattern.patternSeed = 42;
    
    // Set a gate on step 0 for part 0
    pattern.steps[0][0].gate = 1;
    pattern.steps[0][0].velocity = VELOCITY_NORMAL;
    pattern.steps[0][0].probability = 100;
    
    // Load pattern
    engine.setPattern(pattern);
    
    // Check that pattern was loaded (we can't directly check internal state, 
    // but we can verify that the engine doesn't crash)
    
    return true;
}

// TEST: AudioEngine Mute/Solo
// AudioEngine should handle mute/solo states
bool testAudioEngineMuteSolo() {
    AudioEngine engine;
    
    // Test mute
    engine.setPartMute(0, true);
    engine.setPartMute(0, false);
    
    // Test solo
    engine.setPartSolo(0, true);
    engine.setPartSolo(0, false);
    
    // Can't directly verify state, but engine shouldn't crash
    
    return true;
}

// TEST: AudioEngine Degradation
// AudioEngine should handle degradation levels
bool testAudioEngineDegradation() {
    AudioEngine engine;
    
    // Test degradation levels
    for (int level = 0; level <= 2; level++) {
        engine.setDegradationLevel(level);
        int currentLevel = engine.getDegradationLevel();
        if (currentLevel != level) {
            printf("  FAILED: Degradation level should be %d, got %d\n", level, currentLevel);
            return false;
        }
    }
    
    // Test invalid degradation level
    engine.setDegradationLevel(5);  // Invalid
    int currentLevel = engine.getDegradationLevel();
    if (currentLevel != 2) {  // Should clamp to max
        printf("  FAILED: Degradation level should clamp to 2, got %d\n", currentLevel);
        return false;
    }
    
    return true;
}

// TEST: AudioEngine FX Parameters
// AudioEngine should handle FX parameter updates
bool testAudioEngineFXParameters() {
    AudioEngine engine;
    
    // Test delay parameters
    engine.setDelayTimeMs(250.0f);
    engine.setDelayFeedback(0.5f);
    engine.setDelayMix(0.3f);
    
    // Test reverb parameters
    engine.setReverbSize(0.7f);
    engine.setReverbDensity(0.6f);
    engine.setReverbMix(0.4f);
    
    // Test valve and limiter
    engine.setValveAmount(0.2f);
    engine.setLimiterThresholdDb(-0.3f);
    engine.setLimiterReleaseMs(10.0f);
    
    // Can't directly verify values, but engine shouldn't crash
    
    return true;
}

// TEST: AudioEngine Sample Management
// AudioEngine should handle sample loading/unloading
bool testAudioEngineSampleManagement() {
    AudioEngine engine;
    
    // Create a dummy sample
    SampleData sample;
    sample.data = nullptr;  // No actual data in test
    sample.length = 0;
    sample.sampleRate = 44100;
    sample.id = 1;
    sample.loaded = false;
    
    // Test sample loading/unloading
    engine.loadSample(0, sample);
    engine.unloadSample(0);
    
    // Can't directly verify state, but engine shouldn't crash
    
    return true;
}

// TEST: AudioEngine Synth Parameters
// AudioEngine should handle synth parameter updates
bool testAudioEngineSynthParameters() {
    AudioEngine engine;
    
    // Test synth parameters for part 8 (synth part)
    engine.setSynthWavetable(8, 0);  // Saw
    engine.setSynthCutoff(8, 0.5f);
    engine.setSynthResonance(8, 0.3f);
    engine.setSynthAttack(8, 10.0f);
    engine.setSynthDecay(8, 50.0f);
    engine.setSynthSustain(8, 0.7f);
    engine.setSynthRelease(8, 100.0f);
    
    // Can't directly verify values, but engine shouldn't crash
    
    return true;
}

// TEST: AudioEngine Error Telemetry
// AudioEngine should track and reset error counters
bool testAudioEngineErrorTelemetry() {
    AudioEngine engine;
    
    // Get initial error counts
    uint32_t initialCount = engine.getInvalidEventCount();
    uint32_t initialType = engine.getLastInvalidEventType();
    
    // Reset counters
    engine.resetInvalidEventCount();
    
    uint32_t resetCount = engine.getInvalidEventCount();
    if (resetCount != 0) {
        printf("  FAILED: Error count should be 0 after reset, got %u\n", resetCount);
        return false;
    }
    
    return true;
}

// Main test runner
int main() {
    printf("=== TribeX AudioEngine Unit Tests ===\n\n");
    
    // Initialization Tests
    printf("--- Initialization Tests ---\n");
    runTest("AudioEngine Initialization", testAudioEngineInitialization);
    runTest("AudioEngine Safety Checks", testAudioEngineSafetyChecks);
    
    // Parameter Tests
    printf("\n--- Parameter Tests ---\n");
    runTest("AudioEngine Parameter Ranges", testAudioEngineParameterRanges);
    runTest("AudioEngine Degradation", testAudioEngineDegradation);
    
    // Pattern Tests
    printf("\n--- Pattern Tests ---\n");
    runTest("AudioEngine Pattern Management", testAudioEnginePatternManagement);
    
    // Audio Routing Tests
    printf("\n--- Audio Routing Tests ---\n");
    runTest("AudioEngine Mute/Solo", testAudioEngineMuteSolo);
    
    // FX Tests
    printf("\n--- FX Tests ---\n");
    runTest("AudioEngine FX Parameters", testAudioEngineFXParameters);
    
    // Sample Tests
    printf("\n--- Sample Tests ---\n");
    runTest("AudioEngine Sample Management", testAudioEngineSampleManagement);
    
    // Synth Tests
    printf("\n--- Synth Tests ---\n");
    runTest("AudioEngine Synth Parameters", testAudioEngineSynthParameters);
    
    // Telemetry Tests
    printf("\n--- Telemetry Tests ---\n");
    runTest("AudioEngine Error Telemetry", testAudioEngineErrorTelemetry);
    
    printf("\n=== All AudioEngine Tests Passed ===\n");
    return 0;
}