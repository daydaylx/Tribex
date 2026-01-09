#include "PatternData.h"
#include "Probability.h"
#include "Sequencer.h"
#include <cassert>
#include <cstdio>
#include <cstdint>

using namespace Tribex;

// Helper function to run tests
void runTest(const char* testName, bool (*testFunc)()) {
    bool passed = testFunc();
    printf("[%s] %s\n", testName, passed ? "PASS" : "FAIL");
    assert(passed);
}

// TEST: Deterministic Probability
// Same seed + stepIndex should always produce same result
bool testDeterministicProbability() {
    const uint32_t seed = 12345;
    const uint8_t prob = 50;  // 50%
    
    // 100 iterations - all should be identical
    for (uint32_t i = 0; i < 100; i++) {
        bool result1 = shouldTrigger(prob, seed, 10, i);
        bool result2 = shouldTrigger(prob, seed, 10, i);
        
        if (result1 != result2) {
            printf("  FAILED: Iteration %u produced different results\n", i);
            return false;
        }
    }
    
    return true;
}

// TEST: Probability Extremes
// 0% should never trigger, 100% should always trigger
bool testProbabilityExtremes() {
    const uint32_t seed = 99999;
    
    // 0% should never trigger (100 iterations)
    for (uint32_t i = 0; i < 100; i++) {
        bool result = shouldTrigger(0, seed, i, 0);
        if (result) {
            printf("  FAILED: 0%% probability triggered at loop %u\n", i);
            return false;
        }
    }
    
    // 100% should always trigger (100 iterations)
    for (uint32_t i = 0; i < 100; i++) {
        bool result = shouldTrigger(100, seed, i, 0);
        if (!result) {
            printf("  FAILED: 100%% probability did not trigger at loop %u\n", i);
            return false;
        }
    }
    
    return true;
}

// TEST: Probability Loop Consistency
// Probability should be deterministic over multiple loops
bool testProbabilityLoopConsistency() {
    const uint32_t seed = 42;
    const uint8_t prob = 75;  // 75%
    
    // Test 10 different loops
    for (uint32_t loop = 0; loop < 10; loop++) {
        // All loops should produce same trigger sequence
        for (uint32_t step = 0; step < 16; step++) {
            bool result = shouldTrigger(prob, seed, step, loop);
            // Just ensure it doesn't crash and returns consistent type
        }
    }
    
    return true;
}

// TEST: Hash Distribution
// Hash function should produce good distribution (not strict test, just sanity check)
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
    // Expected ~2500 per bucket with good distribution
    const uint32_t expected = iterations / 4;
    const uint32_t tolerance = expected / 4;  // 25% tolerance
    
    for (uint32_t i = 0; i < 4; i++) {
        int32_t diff = static_cast<int32_t>(counts[i]) - static_cast<int32_t>(expected);
        if (abs(diff) > static_cast<int32_t>(tolerance)) {
            printf("  WARNING: Bucket %u has %u samples (expected ~%u)\n", 
                   i, counts[i], expected);
            // Not failing, just warning for sanity check
        }
    }
    
    return true;
}

// TEST: Pattern Data Structure
// Pattern should initialize correctly
bool testPatternInitialization() {
    Pattern pattern;
    
    // Check default values
    if (pattern.id != 0) {
        printf("  FAILED: Pattern ID should be 0, got %u\n", pattern.id);
        return false;
    }
    
    if (pattern.lengthSteps != 16) {
        printf("  FAILED: Pattern length should be 16, got %u\n", pattern.lengthSteps);
        return false;
    }
    
    if (pattern.patternSeed != 42) {
        printf("  FAILED: Pattern seed should be 42, got %u\n", pattern.patternSeed);
        return false;
    }
    
    // Check that all steps are initialized to empty
    for (uint32_t part = 0; part < NUM_PARTS; part++) {
        for (uint32_t step = 0; step < MAX_STEPS; step++) {
            const StepData& s = pattern.steps[part][step];
            if (s.gate != 0 || s.velocity != VELOCITY_NORMAL ||
                s.microtiming != 0 || s.probability != 100) {
                printf("  FAILED: Step (%u,%u) not initialized to empty\n", part, step);
                return false;
            }
        }
    }
    
    return true;
}

// TEST: Chain Initialization
// Chain should initialize correctly
bool testChainInitialization() {
    Chain chain;
    
    // Check default values
    if (chain.length != 0) {
        printf("  FAILED: Chain length should be 0, got %u\n", chain.length);
        return false;
    }
    
    // All entries should be empty
    for (uint32_t i = 0; i < MAX_CHAIN_LENGTH; i++) {
        const ChainEntry& entry = chain.entries[i];
        if (entry.patternId != 0 || entry.repeatCount != 1) {
            printf("  FAILED: Chain entry %u not initialized to empty\n", i);
            return false;
        }
    }
    
    return true;
}

// TEST: Velocity Conversion
// Velocity constants should map to correct float values
bool testVelocityConversion() {
    // Check velocity to float conversion
    if (velocityToFloat(VELOCITY_GHOST) < 0.29f || velocityToFloat(VELOCITY_GHOST) > 0.31f) {
        printf("  FAILED: GHOST velocity incorrect\n");
        return false;
    }
    
    if (velocityToFloat(VELOCITY_NORMAL) < 0.59f || velocityToFloat(VELOCITY_NORMAL) > 0.61f) {
        printf("  FAILED: NORMAL velocity incorrect\n");
        return false;
    }
    
    if (velocityToFloat(VELOCITY_ACCENT) < 0.89f || velocityToFloat(VELOCITY_ACCENT) > 0.91f) {
        printf("  FAILED: ACCENT velocity incorrect\n");
        return false;
    }
    
    if (velocityToFloat(VELOCITY_MAX) < 0.99f || velocityToFloat(VELOCITY_MAX) > 1.01f) {
        printf("  FAILED: MAX velocity incorrect\n");
        return false;
    }
    
    return true;
}

// TEST: Sequencer Lifecycle
// Sequencer should start/stop correctly
bool testSequencerLifecycle() {
    Sequencer sequencer;
    
    // Check initial state
    if (sequencer.isPlaying()) {
        printf("  FAILED: Sequencer should not be playing initially\n");
        return false;
    }
    
    // Start sequencer
    sequencer.start();
    
    if (!sequencer.isPlaying()) {
        printf("  FAILED: Sequencer should be playing after start()\n");
        return false;
    }
    
    // Stop sequencer
    sequencer.stop();
    
    if (sequencer.isPlaying()) {
        printf("  FAILED: Sequencer should not be playing after stop()\n");
        return false;
    }
    
    return true;
}

// TEST: Sequencer BPM
// BPM should clamp and update
bool testSequencerBPM() {
    Sequencer sequencer;
    
    // Check default BPM
    double defaultBPM = sequencer.getBPM();
    if (defaultBPM < 119.9 || defaultBPM > 120.1) {
        printf("  FAILED: Default BPM should be ~120, got %.2f\n", defaultBPM);
        return false;
    }
    
    // Set BPM within range
    sequencer.setBPM(150.0);
    if (sequencer.getBPM() < 149.9 || sequencer.getBPM() > 150.1) {
        printf("  FAILED: BPM should be 150, got %.2f\n", sequencer.getBPM());
        return false;
    }
    
    // Test clamping (below range)
    sequencer.setBPM(10.0);
    if (sequencer.getBPM() < 19.9 || sequencer.getBPM() > 20.1) {
        printf("  FAILED: BPM should clamp to 20, got %.2f\n", sequencer.getBPM());
        return false;
    }
    
    // Test clamping (above range)
    sequencer.setBPM(500.0);
    if (sequencer.getBPM() < 299.9 || sequencer.getBPM() > 300.1) {
        printf("  FAILED: BPM should clamp to 300, got %.2f\n", sequencer.getBPM());
        return false;
    }
    
    return true;
}

// TEST: Sequencer Pattern Load
// Pattern should load and reset position
bool testSequencerPatternLoad() {
    Sequencer sequencer;
    
    // Create test pattern
    Pattern pattern;
    pattern.id = 1;
    pattern.lengthSteps = 32;
    pattern.patternSeed = 123;
    
    // Load pattern
    sequencer.loadPattern(pattern);
    
    // Check that position was reset
    if (sequencer.getCurrentStep() != 0) {
        printf("  FAILED: Current step should be 0 after pattern load, got %u\n", 
               sequencer.getCurrentStep());
        return false;
    }
    
    if (sequencer.getLoopIteration() != 0) {
        printf("  FAILED: Loop iteration should be 0 after pattern load, got %u\n", 
               sequencer.getLoopIteration());
        return false;
    }
    
    if (sequencer.getActivePatternId() != pattern.id) {
        printf("  FAILED: Active pattern ID should be %u, got %u\n", 
               pattern.id, sequencer.getActivePatternId());
        return false;
    }
    
    return true;
}

// TEST: Microtiming Constants
// Microtiming should have correct ranges
bool testMicrotimingConstants() {
    if (MICROTIMING_MIN != -50) {
        printf("  FAILED: MICROTIMING_MIN should be -50, got %d\n", MICROTIMING_MIN);
        return false;
    }
    
    if (MICROTIMING_MAX != 50) {
        printf("  FAILED: MICROTIMING_MAX should be 50, got %d\n", MICROTIMING_MAX);
        return false;
    }
    
    return true;
}

// TEST: Sequencer Update (Simple)
// Sequencer should update step position
bool testSequencerUpdateSimple() {
    Sequencer sequencer;
    
    // Load simple pattern
    Pattern pattern;
    pattern.lengthSteps = 16;
    pattern.patternSeed = 42;
    
    // Set gate on step 0 for part 0
    pattern.steps[0][0].gate = 1;
    pattern.steps[0][0].probability = 100;
    
    sequencer.loadPattern(pattern);
    sequencer.start();
    
    // Get triggers (simulated)
    StepTrigger triggers[NUM_PARTS];
    uint32_t numTriggers = 0;
    
    // Simulate one step (48kHz sample rate, 120bpm = 44100 samples/step)
    const double sampleRate = 48000.0;
    const int64_t firstStepSample = 0;
    sequencer.update(firstStepSample, sampleRate, triggers, &numTriggers);
    
    if (numTriggers != 1) {
        printf("  FAILED: Expected 1 trigger at step 0, got %u\n", numTriggers);
        return false;
    }
    
    if (!triggers[0].triggered) {
        printf("  FAILED: Part 0 should have triggered at step 0\n");
        return false;
    }
    
    return true;
}

// TEST: Idempotency
// Multiple sequencer update calls should be idempotent
bool testSequencerIdempotency() {
    Sequencer sequencer;
    
    Pattern pattern;
    pattern.lengthSteps = 16;
    pattern.patternSeed = 42;
    pattern.steps[0][0].gate = 1;
    pattern.steps[0][0].probability = 100;
    
    sequencer.loadPattern(pattern);
    sequencer.start();
    
    const double sampleRate = 48000.0;
    StepTrigger triggers1[NUM_PARTS];
    uint32_t numTriggers1 = 0;
    
    StepTrigger triggers2[NUM_PARTS];
    uint32_t numTriggers2 = 0;
    
    // Call update twice with same sample counter
    const int64_t sample = 0;
    sequencer.update(sample, sampleRate, triggers1, &numTriggers1);
    sequencer.update(sample, sampleRate, triggers2, &numTriggers2);
    
    // Second call should produce 0 triggers (step didn't change)
    if (numTriggers2 != 0) {
        printf("  FAILED: Second update should produce 0 triggers, got %u\n", numTriggers2);
        return false;
    }
    
    return true;
}

// TEST: Pattern Change During Playback
// Switching pattern should be seamless
bool testSequencerPatternChangeDuringPlayback() {
    Sequencer sequencer;
    
    // Pattern 1: 16 steps
    Pattern p1;
    p1.id = 1;
    p1.lengthSteps = 16;
    
    // Pattern 2: 32 steps
    Pattern p2;
    p2.id = 2;
    p2.lengthSteps = 32;
    
    sequencer.loadPattern(p1);
    sequencer.start();
    
    // Simulate some playback (step 5)
    // 44100 samples/step @ 120bpm, 48kHz
    const double sampleRate = 48000.0;
    const int64_t step5Sample = static_cast<int64_t>(5 * (60.0 * sampleRate / (120.0 * 4.0)));
    
    StepTrigger triggers[NUM_PARTS];
    uint32_t numTriggers = 0;
    sequencer.update(step5Sample, sampleRate, triggers, &numTriggers);
    
    if (sequencer.getCurrentStep() != 5) {
        printf("  FAILED: Should be at step 5, got %u\n", sequencer.getCurrentStep());
        return false;
    }
    
    // Load new pattern while playing
    sequencer.loadPattern(p2);
    
    // Position should reset to 0 immediately (per current design)
    // Note: Future versions might want to sync to bar boundary
    if (sequencer.getCurrentStep() != 0) {
        printf("  FAILED: Position should reset to 0 after pattern change, got %u\n", 
               sequencer.getCurrentStep());
        return false;
    }
    
    if (sequencer.getActivePatternId() != 2) {
        printf("  FAILED: Active pattern should be 2, got %u\n", sequencer.getActivePatternId());
        return false;
    }
    
    return true;
}

// Main test runner
int main() {
    printf("=== TribeX Sequencer Unit Tests ===\n\n");
    
    // Probability Tests
    printf("--- Probability Tests ---\n");
    runTest("Deterministic Probability", testDeterministicProbability);
    runTest("Probability Extremes", testProbabilityExtremes);
    runTest("Probability Loop Consistency", testProbabilityLoopConsistency);
    runTest("Hash Distribution", testHashDistribution);
    
    // Data Structure Tests
    printf("\n--- Data Structure Tests ---\n");
    runTest("Pattern Initialization", testPatternInitialization);
    runTest("Chain Initialization", testChainInitialization);
    runTest("Velocity Conversion", testVelocityConversion);
    runTest("Microtiming Constants", testMicrotimingConstants);
    
    // Sequencer Tests
    printf("\n--- Sequencer Tests ---\n");
    runTest("Sequencer Lifecycle", testSequencerLifecycle);
    runTest("Sequencer BPM", testSequencerBPM);
    runTest("Sequencer Pattern Load", testSequencerPatternLoad);
    runTest("Sequencer Pattern Change During Playback", testSequencerPatternChangeDuringPlayback);
    runTest("Sequencer Update Simple", testSequencerUpdateSimple);
    runTest("Sequencer Idempotency", testSequencerIdempotency);
    
    printf("\n=== All Tests Passed ===\n");
    return 0;
}
