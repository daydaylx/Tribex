#include "SamplePart.h"
#include "SampleVoice.h"
#include <cassert>
#include <cstdio>
#include <cstdint>

using namespace Tribex;

using namespace Tribex;

// Helper function to run tests
void runTest(const char* testName, bool (*testFunc)()) {
    bool passed = testFunc();
    printf("[%s] %s\n", testName, passed ? "PASS" : "FAIL");
    assert(passed);
}

// TEST: SamplePart Initialization
// SamplePart should initialize correctly
bool testSamplePartInitialization() {
    SamplePart part(0);
    
    // Check initial state
    if (part.hasSample()) {
        printf("  FAILED: SamplePart should not have sample initially\n");
        return false;
    }
    
    if (part.isMuted()) {
        printf("  FAILED: SamplePart should not be muted initially\n");
        return false;
    }
    
    if (part.isSoloed()) {
        printf("  FAILED: SamplePart should not be soloed initially\n");
        return false;
    }
    
    return true;
}

// TEST: SamplePart Parameter Updates
// SamplePart should handle parameter updates
bool testSamplePartParameterUpdates() {
    SamplePart part(0);
    
    // Test parameter updates (can't verify values directly, but shouldn't crash)
    part.setPitch(1.0f);
    part.setPan(0.0f);
    part.setLevel(1.0f);
    part.setDecay(100.0f);
    part.setFilter(FilterType::LP);
    
    return true;
}

// TEST: SamplePart Mute/Solo
// SamplePart should handle mute/solo states
bool testSamplePartMuteSolo() {
    SamplePart part(0);
    
    // Test mute
    part.setMute(true);
    if (!part.isMuted()) {
        printf("  FAILED: SamplePart should be muted\n");
        return false;
    }
    
    part.setMute(false);
    if (part.isMuted()) {
        printf("  FAILED: SamplePart should not be muted\n");
        return false;
    }
    
    // Test solo
    part.setSolo(true);
    if (!part.isSoloed()) {
        printf("  FAILED: SamplePart should be soloed\n");
        return false;
    }
    
    part.setSolo(false);
    if (part.isSoloed()) {
        printf("  FAILED: SamplePart should not be soloed\n");
        return false;
    }
    
    return true;
}

// TEST: SamplePart Sample Loading
// SamplePart should handle sample loading/unloading
bool testSamplePartSampleLoading() {
    SamplePart part(0);
    
    // Create a dummy sample
    SampleData sample;
    sample.data = nullptr;  // No actual data in test
    sample.length = 0;
    sample.sampleRate = 44100;
    sample.id = 1;
    sample.loaded = false;
    
    // Test sample loading/unloading
    part.loadSample(sample, false);
    part.unloadSample(false);
    
    // Can't directly verify state, but part shouldn't crash
    
    return true;
}

// TEST: SamplePart Trigger
// SamplePart should handle trigger calls
bool testSamplePartTrigger() {
    SamplePart part(0);
    
    // Test trigger (should return false since no sample is loaded)
    bool triggered = part.trigger(1.0f);
    if (triggered) {
        printf("  FAILED: SamplePart should not trigger without sample\n");
        return false;
    }
    
    return true;
}

// TEST: SamplePart Render
// SamplePart should handle render calls
bool testSamplePartRender() {
    SamplePart part(0);
    
    // Create temporary buffers
    const int32_t numFrames = 64;
    float leftBuffer[numFrames] = {0.0f};
    float rightBuffer[numFrames] = {0.0f};
    
    // Test render (should not crash)
    part.render(leftBuffer, rightBuffer, numFrames);
    
    return true;
}

// Main test runner
int main() {
    printf("=== TribeX SamplePart Unit Tests ===\n\n");
    
    // Initialization Tests
    printf("--- Initialization Tests ---\n");
    runTest("SamplePart Initialization", testSamplePartInitialization);
    
    // Parameter Tests
    printf("\n--- Parameter Tests ---\n");
    runTest("SamplePart Parameter Updates", testSamplePartParameterUpdates);
    runTest("SamplePart Mute/Solo", testSamplePartMuteSolo);
    
    // Sample Management Tests
    printf("\n--- Sample Management Tests ---\n");
    runTest("SamplePart Sample Loading", testSamplePartSampleLoading);
    
    // Audio Tests
    printf("\n--- Audio Tests ---\n");
    runTest("SamplePart Trigger", testSamplePartTrigger);
    runTest("SamplePart Render", testSamplePartRender);
    
    printf("\n=== All SamplePart Tests Passed ===\n");
    return 0;
}