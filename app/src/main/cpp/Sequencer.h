#ifndef TRIBEX_SEQUENCER_H
#define TRIBEX_SEQUENCER_H

#include "PatternData.h"
#include "Probability.h"
#include <atomic>
#include <cstdint>

namespace Tribex {

/**
 * Sequencer Engine - Core Timing and Step Evaluation
 * 
 * This is called from audio thread - NO ALLOCATIONS!
 * 
 * Timing is based on sampleCounter (int64), not wall-clock.
 * This ensures sample-accurate timing and determinism.
 * 
 * The sequencer maintains:
 * - Current step position
 * - Loop iteration count
 * - Active pattern and chain
 * - Trigger events for each part
 */

// Sequencer Configuration
constexpr double DEFAULT_BPM = 120.0;
constexpr double STEPS_PER_BEAT = 4.0;  // 16th notes

// Trigger Event for one part
struct StepTrigger {
    uint32_t partIndex;  // 0-8 (0-7 = drums, 8 = synth)
    uint32_t stepIndex;  // Current step index
    uint8_t velocity;    // 2-bit velocity
    bool triggered;       // Whether this step triggered
    
    StepTrigger()
        : partIndex(0)
        , stepIndex(0)
        , velocity(VELOCITY_NORMAL)
        , triggered(false)
    {}
};

class Sequencer {
public:
    Sequencer();
    ~Sequencer() = default;
    
    // Lifecycle
    void start();
    void stop();
    bool isPlaying() const { return mIsPlaying.load(std::memory_order_relaxed); }
    
    // Pattern/Chain Management
    void loadPattern(const Pattern& pattern);
    void loadChain(const Chain& chain);
    
    // Timing
    void setBPM(double bpm);
    double getBPM() const { return mBPM.load(std::memory_order_relaxed); }
    
    // Main Update Method (called from audio thread)
    // Updates sequencer state and returns trigger events
    // CRITICAL: NO ALLOCATIONS!
    void update(int64_t sampleCounter, double sampleRate, StepTrigger* triggers, uint32_t* numTriggers);
    
    // State
    uint32_t getCurrentStep() const { return mCurrentStepIndex.load(std::memory_order_relaxed); }
    uint32_t getLoopIteration() const { return mLoopIteration.load(std::memory_order_relaxed); }
    
    // Pattern Management
    uint32_t getActivePatternId() const { return mActivePattern.id; }
    uint32_t getPatternLength() const { return mActivePattern.lengthSteps; }
    
private:
    // Calculate samples per step based on BPM and sample rate
    double calculateSamplesPerStep(double sampleRate) const;
    
    // Evaluate step trigger for one part
    bool evaluateStepTrigger(uint32_t partIndex, uint32_t stepIndex, uint32_t loopIteration);
    
    // Apply microtiming offset
    int64_t applyMicrotiming(int64_t sampleCounter, int8_t microtiming, double sampleRate);
    
    // Calculate current step index from sample counter
    void calculateCurrentStep(int64_t sampleCounter, double sampleRate);
    
    // Check if step has changed
    bool hasStepChanged() const;
    
    // Atomic state
    std::atomic<bool> mIsPlaying;
    std::atomic<double> mBPM;
    
    // Sequencer state
    std::atomic<uint32_t> mCurrentStepIndex;
    std::atomic<uint32_t> mPreviousStepIndex;
    std::atomic<uint32_t> mLoopIteration;
    std::atomic<int64_t> mLastStepSampleCounter;
    
    // Active pattern and chain
    Pattern mActivePattern;
    Chain mActiveChain;
    
    // Chain playback state
    uint32_t mChainEntryIndex;
    uint32_t mChainRepeatCounter;
    
    // Samples per step (calculated from BPM)
    double mSamplesPerStep;
};

} // namespace Tribex

#endif // TRIBEX_SEQUENCER_H