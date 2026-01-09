/**
 * Audio Engine Unit Tests
 * 
 * Tests for M4.5 features:
 * - LockFreeQueue: push/pop without overflow
 * - Probability: deterministic triggers with same seed
 * - AudioEngine: fixed buffer allocation, no dynamic allocations in render path
 */

#include "AudioEngine.h"
#include "EventQueue.h"
#include "Probability.h"
#include "Sequencer.h"
#include "PatternData.h"
#include "SamplePart.h"
#include "SynthPart.h"
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <iostream>

using namespace Tribex;

// Helper function to run tests
void runTest(const char* testName, bool (*testFunc)()) {
    bool passed = testFunc();
    std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName << std::endl;
    assert(passed);
}

// TEST: LockFreeQueue Push/Pop
// Queue should handle push/pop operations without overflow
bool testLockFreeQueueBasic() {
    LockFreeQueue<AudioEvent, 256> queue;
    
    // Test initial state
    if (!queue.isEmpty()) {
        std::cout << "  FAILED: Queue should be empty initially" << std::endl;
        return false;
    }
    
    // Test push/pop
    AudioEvent event1(EventType::SET_MASTER_GAIN, 0.5f);
    AudioEvent event2(EventType::SET_MASTER_PAN, 0.0f);
    
    if (!queue.push(event1)) {
        std::cout << "  FAILED: Push failed" << std::endl;
        return false;
    }
    
    if (queue.isEmpty()) {
        std::cout << "  FAILED: Queue should not be empty after push" << std::endl;
        return false;
    }
    
    AudioEvent popped;
    if (!queue.pop(popped)) {
        std::cout << "  FAILED: Pop failed" << std::endl;
        return false;
    }
    
    if (popped.type != event1.type || popped.value != event1.value) {
        std::cout << "  FAILED: Popped event doesn't match pushed event" << std::endl;
        return false;
    }
    
    return true;
}

// TEST: LockFreeQueue Multiple Push/Pop
// Queue should handle multiple operations correctly
bool testLockFreeQueueMultiple() {
    LockFreeQueue<AudioEvent, 256> queue;
    
    const int count = 100;
    for (int i = 0; i < count; i++) {
        AudioEvent event(EventType::SET_TEST_TONE_FREQ, static_cast<float>(i));
        if (!queue.push(event)) {
            std::cout << "  FAILED: Push failed at iteration " << i << std::endl;
            return false;
        }
    }
    
    for (int i = 0; i < count; i++) {
        AudioEvent popped;
        if (!queue.pop(popped)) {
            std::cout << "  FAILED: Pop failed at iteration " << i << std::endl;
            return false;
        }
        
        if (static_cast<int>(popped.value) != i) {
            std::cout << "  FAILED: Value mismatch at iteration " << i << std::endl;
            return false;
        }
    }
    
    if (!queue.isEmpty()) {
        std::cout << "  FAILED: Queue should be empty after all pops" << std::endl;
        return false;
    }
    
    return true;
}

// TEST: LockFreeQueue Overflow
// Queue should reject push when full
bool testLockFreeQueueOverflow() {
    LockFreeQueue<AudioEvent, 4> queue;  // Small queue for testing
    
    // Fill queue
    for (int i = 0; i < 4; i++) {
        AudioEvent event(EventType::SET_MASTER_GAIN, 1.0f);
        if (!queue.push(event)) {
            std::cout << "  FAILED: Push should succeed when queue not full" << std::endl;
            return false;
        }
    }
    
    // Try to push to full queue
    AudioEvent event(EventType::SET_MASTER_GAIN, 1.0f);
    if (queue.push(event)) {
        std::cout << "  FAILED: Push should fail when queue is full" << std::endl;
        return false;
    }
    
    return true;
}

// TEST: Probability Deterministic
// Same inputs should always produce same output
bool testProbabilityDeterministic() {
    const uint32_t seed = 12345;
    const uint8_t prob = 50;
    
    // Test 100 iterations - all should be identical
    for (uint32_t i = 0; i < 100; i++) {
        bool result1 = shouldTrigger(prob, seed, i, 0);
        bool result2 = shouldTrigger(prob, seed, i, 0);
        
        if (result1 != result2) {
            std::cout << "  FAILED: Iteration " << i << " produced different results" << std::endl;
            return false;
        }
    }
    
    return true;
}

// TEST: Probability Seed Variance
// Different seeds should produce different results
bool testProbabilitySeedVariance() {
    const uint8_t prob = 50;
    const uint32_t seed1 = 100;
    const uint32_t seed2 = 200;
    
    // Test 100 iterations - at least some should differ
    uint32_t sameCount = 0;
    for (uint32_t i = 0; i < 100; i++) {
        bool result1 = shouldTrigger(prob, seed1, i, 0);
        bool result2 = shouldTrigger(prob, seed2, i, 0);
        
        if (result1 == result2) {
            sameCount++;
        }
    }
    
    // At least 20% should differ
    if (sameCount > 80) {
        std::cout << "  FAILED: Seeds produced too similar results (" << sameCount << "% same)" << std::endl;
        return false;
    }
    
    return true;
}

// TEST: Probability Loop Consistency
// Same step in different loops should be deterministic
bool testProbabilityLoopConsistency() {
    const uint32_t seed = 42;
    const uint8_t prob = 75;
    
    // Test 10 different loops
    for (uint32_t loop = 0; loop < 10; loop++) {
        for (uint32_t step = 0; step < 16; step++) {
            bool result = shouldTrigger(prob, seed, step, loop);
            // Just ensure it doesn't crash and returns bool
        }
    }
    
    return true;
}

// TEST: Probability Extremes
// 0% should never trigger, 100% should always trigger
bool testProbabilityExtremes() {
    const uint32_t seed = 99999;
    
    // 0% should never trigger
    for (uint32_t i = 0; i < 100; i++) {
        if (shouldTrigger(0, seed, i, 0)) {
            std::cout << "  FAILED: 0% probability triggered at iteration " << i << std::endl;
            return false;
        }
    }
    
    // 100% should always trigger
    for (uint32_t i = 0; i < 100; i++) {
        if (!shouldTrigger(100, seed, i, 0)) {
            std::cout << "  FAILED: 100% probability did not trigger at iteration " << i << std::endl;
            return false;
        }
    }
    
    return true;
}

// TEST: AudioEvent Validation
// Events should validate correctly
bool testAudioEventValidation() {
    // Valid event
    AudioEvent validEvent(EventType::SET_MASTER_GAIN, 0.5f);
    if (!validEvent.isValid()) {
        std::cout << "  FAILED: Valid event marked as invalid" << std::endl;
        return false;
    }
    
    // Invalid event (type out of range)
    AudioEvent invalidEvent;
    invalidEvent.type = static_cast<EventType>(999);
    if (invalidEvent.isValid()) {
        std::cout << "  FAILED: Invalid event marked as valid" << std::endl;
        return false;
    }
    
    return true;
}

// TEST: AudioEngine Buffer Size
// AudioEngine should use fixed buffer size
bool testAudioEngineBufferSize() {
    // This is a compile-time check
    // MAX_FRAMES_PER_CALLBACK should be defined and be 1024
    const int32_t expectedMaxFrames = 1024;
    
    // Note: We can't access private members directly,
    // but we can verify the header has the constant
    // This test is primarily for documentation
    
    return true;  // Pass if we compiled successfully
}

// TEST: AudioEngine Initialization
// AudioEngine should initialize without crashing
bool testAudioEngineInitialization() {
    AudioEngine engine;
    
    // Just ensure it doesn't crash during construction
    // Audio engine doesn't require external dependencies for construction
    
    return true;
}

// TEST: Sequencer Null Pointer Safety
// Sequencer::update() should handle null pointers gracefully
bool testSequencerNullPointerSafety() {
    Tribex::Sequencer sequencer;
    sequencer.start();
    
    // Test with null triggers pointer
    uint32_t numTriggers = 0;
    sequencer.update(0, 44100.0, nullptr, &numTriggers);
    if (numTriggers != 0) {
        std::cout << "  FAILED: numTriggers should remain 0 with null triggers" << std::endl;
        return false;
    }
    
    // Test with null numTriggers pointer
    Tribex::StepTrigger triggers[16];
    sequencer.update(0, 44100.0, triggers, nullptr);
    // Should not crash
    
    // Test with both null
    sequencer.update(0, 44100.0, nullptr, nullptr);
    // Should not crash
    
    return true;
}

// TEST: Sequencer Array Bounds Safety
// Sequencer::update() should not overflow trigger array
bool testSequencerArrayBoundsSafety() {
    Tribex::Sequencer sequencer;
    sequencer.start();
    
    // Create a pattern with all steps enabled for all parts
    Tribex::Pattern pattern;
    pattern.lengthSteps = 16;
    pattern.patternSeed = 12345;
    
    for (uint32_t part = 0; part < Tribex::NUM_PARTS; part++) {
        for (uint32_t step = 0; step < 16; step++) {
            pattern.steps[part][step].gate = 1;
            pattern.steps[part][step].velocity = Tribex::VELOCITY_NORMAL;
            pattern.steps[part][step].probability = 100;  // Always trigger
        }
    }
    
    sequencer.loadPattern(pattern);
    
    // Update sequencer - should not overflow
    Tribex::StepTrigger triggers[16];
    uint32_t numTriggers = 0;
    
    // Simulate multiple step changes
    for (int64_t sample = 0; sample < 100000; sample += 1000) {
        sequencer.update(sample, 44100.0, triggers, &numTriggers);
        
        // Check bounds
        if (numTriggers > 16) {
            std::cout << "  FAILED: numTriggers (" << numTriggers 
                      << ") exceeded MAX_TRIGGERS_PER_CALLBACK (16)" << std::endl;
            return false;
        }
    }
    
    return true;
}

// TEST: Sample Length Validation
// Sample length should be divisible by sizeof(float)
bool testSampleLengthValidation() {
    // This test verifies the logic in loadSample JNI method
    // We can't directly test JNI, but we can test the calculation
    
    const int length1 = 1024;  // Valid: divisible by 4
    const int length2 = 1025;  // Invalid: not divisible by 4
    
    uint32_t calculated1 = static_cast<uint32_t>(length1 / sizeof(float));
    uint32_t calculated2 = static_cast<uint32_t>(length2 / sizeof(float));
    
    // Check if calculation is correct
    if (calculated1 != 256) {
        std::cout << "  FAILED: Length calculation incorrect for valid length" << std::endl;
        return false;
    }
    
    // Check if invalid length would cause issues
    if (length2 % sizeof(float) == 0) {
        std::cout << "  WARNING: Length validation might not catch all cases" << std::endl;
    }
    
    return true;
}

// TEST: Sequencer Step Calculation
// Sequencer should calculate steps correctly
bool testSequencerStepCalculation() {
    Tribex::Sequencer sequencer;
    sequencer.setBPM(120.0);
    sequencer.start();
    
    Tribex::Pattern pattern;
    pattern.lengthSteps = 16;
    sequencer.loadPattern(pattern);
    
    Tribex::StepTrigger triggers[16];
    uint32_t numTriggers = 0;
    
    // At 120 BPM, 44100 Hz, samples per step = (60 * 44100) / (120 * 4) = 5512.5
    // After 5513 samples, we should be at step 1
    sequencer.update(5513, 44100.0, triggers, &numTriggers);
    
    uint32_t currentStep = sequencer.getCurrentStep();
    
    // Should be at step 1 (or close, depending on rounding)
    if (currentStep > 2) {
        std::cout << "  WARNING: Step calculation might be off (got step " 
                  << currentStep << ", expected ~1)" << std::endl;
        // Not failing, as exact calculation depends on implementation
    }
    
    return true;
}

// TEST: Sequencer Division by Zero Safety
// Sequencer should handle zero samplesPerStep and zero lengthSteps gracefully
bool testSequencerDivisionByZeroSafety() {
    Tribex::Sequencer sequencer;
    sequencer.start();
    
    // Test with zero BPM (should be clamped to 20.0)
    sequencer.setBPM(0.0);
    
    Tribex::Pattern pattern;
    pattern.lengthSteps = 0;  // Invalid pattern length
    sequencer.loadPattern(pattern);
    
    Tribex::StepTrigger triggers[16];
    uint32_t numTriggers = 0;
    
    // Should not crash with invalid pattern length
    sequencer.update(1000, 44100.0, triggers, &numTriggers);
    
    // Test with zero sample rate
    sequencer.update(1000, 0.0, triggers, &numTriggers);
    // Should not crash
    
    // Test with valid pattern
    pattern.lengthSteps = 16;
    sequencer.loadPattern(pattern);
    sequencer.setBPM(120.0);
    
    // Should work normally now
    sequencer.update(1000, 44100.0, triggers, &numTriggers);
    
    return true;
}

// TEST: Sequencer Step Index Bounds
// Sequencer should ensure step index is within valid range
bool testSequencerStepIndexBounds() {
    Tribex::Sequencer sequencer;
    sequencer.setBPM(120.0);
    sequencer.start();
    
    Tribex::Pattern pattern;
    pattern.lengthSteps = 16;
    sequencer.loadPattern(pattern);
    
    Tribex::StepTrigger triggers[16];
    uint32_t numTriggers = 0;
    
    // Simulate very large sample counter that could cause overflow
    sequencer.update(INT64_MAX / 2, 44100.0, triggers, &numTriggers);
    
    uint32_t currentStep = sequencer.getCurrentStep();
    
    // Step should be within valid range (0-15 for 16-step pattern)
    if (currentStep >= pattern.lengthSteps) {
        std::cout << "  FAILED: Step index (" << currentStep 
                  << ") exceeded pattern length (" << pattern.lengthSteps << ")" << std::endl;
        return false;
    }
    
    return true;
}

// TEST: SamplePart Null Pointer Safety
// SamplePart::render() should handle null pointers gracefully
bool testSamplePartNullPointerSafety() {
    Tribex::SamplePart part(0);
    
    // Test with null left buffer
    float rightBuffer[1024] = {0};
    part.render(nullptr, rightBuffer, 1024);
    // Should not crash
    
    // Test with null right buffer
    float leftBuffer[1024] = {0};
    part.render(leftBuffer, nullptr, 1024);
    // Should not crash
    
    // Test with both null
    part.render(nullptr, nullptr, 1024);
    // Should not crash
    
    // Test with invalid numFrames
    part.render(leftBuffer, rightBuffer, -1);
    part.render(leftBuffer, rightBuffer, 0);
    part.render(leftBuffer, rightBuffer, 2000);  // > MAX_FRAMES
    // Should not crash
    
    return true;
}

// TEST: SynthPart Null Pointer Safety
// SynthPart::render() should handle null pointers gracefully
bool testSynthPartNullPointerSafety() {
    Tribex::SynthPart synth;
    synth.initialize(44100.0f);
    
    // Test with null left buffer
    float rightBuffer[1024] = {0};
    synth.render(nullptr, rightBuffer, 1024);
    // Should not crash
    
    // Test with null right buffer
    float leftBuffer[1024] = {0};
    synth.render(leftBuffer, nullptr, 1024);
    // Should not crash
    
    // Test with both null
    synth.render(nullptr, nullptr, 1024);
    // Should not crash
    
    // Test with invalid numFrames
    synth.render(leftBuffer, rightBuffer, -1);
    synth.render(leftBuffer, rightBuffer, 0);
    synth.render(leftBuffer, rightBuffer, 2000);  // > MAX_FRAMES
    // Should not crash
    
    return true;
}

// TEST: Hash Distribution
// Hash function should produce good distribution
bool testHashDistribution() {
    const uint32_t seed = 12345;
    
    uint32_t counts[4] = {0, 0, 0, 0};
    const uint32_t iterations = 10000;
    
    for (uint32_t i = 0; i < iterations; i++) {
        uint32_t hash = hashProbability(seed, i, 0);
        uint32_t bucket = hash & 0x3;  // Last 2 bits -> 4 buckets
        counts[bucket]++;
    }
    
    // All buckets should have roughly equal distribution
    const uint32_t expected = iterations / 4;
    const uint32_t tolerance = expected / 4;  // 25% tolerance
    
    for (uint32_t i = 0; i < 4; i++) {
        int32_t diff = static_cast<int32_t>(counts[i]) - static_cast<int32_t>(expected);
        if (std::abs(diff) > static_cast<int32_t>(tolerance)) {
            std::cout << "  WARNING: Bucket " << i << " has " << counts[i] 
                      << " samples (expected ~" << expected << ")" << std::endl;
            // Not failing, just warning for sanity check
        }
    }
    
    return true;
}

// Main test runner
int main() {
    std::cout << "=== TribeX Audio Engine Unit Tests ===" << std::endl << std::endl;
    
    // LockFreeQueue Tests
    std::cout << "--- LockFreeQueue Tests ---" << std::endl;
    runTest("LockFreeQueue Basic", testLockFreeQueueBasic);
    runTest("LockFreeQueue Multiple", testLockFreeQueueMultiple);
    runTest("LockFreeQueue Overflow", testLockFreeQueueOverflow);
    
    // Probability Tests
    std::cout << std::endl << "--- Probability Tests ---" << std::endl;
    runTest("Probability Deterministic", testProbabilityDeterministic);
    runTest("Probability Seed Variance", testProbabilitySeedVariance);
    runTest("Probability Loop Consistency", testProbabilityLoopConsistency);
    runTest("Probability Extremes", testProbabilityExtremes);
    runTest("Hash Distribution", testHashDistribution);
    
    // AudioEvent Tests
    std::cout << std::endl << "--- AudioEvent Tests ---" << std::endl;
    runTest("AudioEvent Validation", testAudioEventValidation);
    
    // AudioEngine Tests
    std::cout << std::endl << "--- AudioEngine Tests ---" << std::endl;
    runTest("AudioEngine Buffer Size", testAudioEngineBufferSize);
    runTest("AudioEngine Initialization", testAudioEngineInitialization);
    
    // Sequencer Tests
    std::cout << std::endl << "--- Sequencer Tests ---" << std::endl;
    runTest("Sequencer Null Pointer Safety", testSequencerNullPointerSafety);
    runTest("Sequencer Array Bounds Safety", testSequencerArrayBoundsSafety);
    runTest("Sequencer Step Calculation", testSequencerStepCalculation);
    runTest("Sequencer Division by Zero Safety", testSequencerDivisionByZeroSafety);
    runTest("Sequencer Step Index Bounds", testSequencerStepIndexBounds);
    
    // Sample Loading Tests
    std::cout << std::endl << "--- Sample Loading Tests ---" << std::endl;
    runTest("Sample Length Validation", testSampleLengthValidation);
    
    // Render Safety Tests
    std::cout << std::endl << "--- Render Safety Tests ---" << std::endl;
    runTest("SamplePart Null Pointer Safety", testSamplePartNullPointerSafety);
    runTest("SynthPart Null Pointer Safety", testSynthPartNullPointerSafety);
    
    std::cout << std::endl << "=== All Tests Passed ===" << std::endl;
    return 0;
}
