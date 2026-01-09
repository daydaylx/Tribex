#include "Sequencer.h"
#include <algorithm>
#include <cstdio>
#include <ctime>

namespace Tribex {

Sequencer::Sequencer()
    : mIsPlaying(false)
    , mBPM(DEFAULT_BPM)
    , mCurrentStepIndex(0)
    , mPreviousStepIndex(0)
    , mLoopIteration(0)
    , mLastStepSampleCounter(0)
    , mChainEntryIndex(0)
    , mChainRepeatCounter(0)
    , mSamplesPerStep(0.0)
{
    // Initialize with empty pattern
    mPatternBuffers[0] = Pattern();
    mPatternBuffers[1] = Pattern();
    mActivePatternIndex.store(0, std::memory_order_relaxed);
    mActiveChain = Chain();
}

void Sequencer::start() {
    mIsPlaying.store(true, std::memory_order_relaxed);
    
    // Reset step counter on start
    mLastStepSampleCounter.store(0, std::memory_order_relaxed);
    mCurrentStepIndex.store(0, std::memory_order_relaxed);
    mPreviousStepIndex.store(MAX_STEPS, std::memory_order_relaxed);
    mLoopIteration.store(0, std::memory_order_relaxed);
}

void Sequencer::stop() {
    mIsPlaying.store(false, std::memory_order_relaxed);
}

void Sequencer::loadPattern(const Pattern& pattern) {
    uint32_t currentIndex = mActivePatternIndex.load(std::memory_order_acquire);
    uint32_t nextIndex = 1 - currentIndex;
    mPatternBuffers[nextIndex] = pattern;
    mActivePatternIndex.store(nextIndex, std::memory_order_release);
    
    // Reset sequencer position
    mCurrentStepIndex.store(0, std::memory_order_relaxed);
    mPreviousStepIndex.store(0, std::memory_order_relaxed);
    mLoopIteration.store(0, std::memory_order_relaxed);
}

void Sequencer::loadChain(const Chain& chain) {
    mActiveChain = chain;
    
    // Reset chain playback
    mChainEntryIndex = 0;
    mChainRepeatCounter = 0;
}

void Sequencer::setBPM(double bpm) {
    // Clamp BPM to reasonable range
    if (bpm < 20.0) bpm = 20.0;
    if (bpm > 300.0) bpm = 300.0;
    
    mBPM.store(bpm, std::memory_order_relaxed);
}

double Sequencer::calculateSamplesPerStep(double sampleRate) const {
    double bpm = mBPM.load(std::memory_order_relaxed);
    return (60.0 * sampleRate) / (bpm * STEPS_PER_BEAT);
}

void Sequencer::update(int64_t sampleCounter, double sampleRate, StepTrigger* triggers, uint32_t* numTriggers) {
    // CRITICAL: This is called from audio thread - NO ALLOCATIONS!
    
    // Safety check: null pointer validation (early return)
    if (!numTriggers || !triggers) {
        return;
    }
    
    // Initialize numTriggers to 0
    *numTriggers = 0;
    
    // Check if sequencer is playing
    bool isPlaying = mIsPlaying.load(std::memory_order_relaxed);
    if (!isPlaying) {
        return;
    }
    
    // Safety check: validate sample rate
    if (sampleRate <= 0.0) {
        return;
    }

    const Pattern& pattern = mPatternBuffers[mActivePatternIndex.load(std::memory_order_acquire)];

    // Update samples per step (in case BPM changed)
    mSamplesPerStep = calculateSamplesPerStep(sampleRate);
    
    // Safety check: ensure samples per step is valid
    if (mSamplesPerStep <= 0.0) {
        return;
    }
    
    // Calculate current step
    calculateCurrentStep(sampleCounter, sampleRate, pattern.lengthSteps);
    
    // Check if step changed
    bool stepChanged = hasStepChanged();
    if (!stepChanged) {
        return;
    }
    
    // Step changed - evaluate triggers for all parts
    uint32_t currentStep = mCurrentStepIndex.load(std::memory_order_relaxed);
    uint32_t loopIteration = mLoopIteration.load(std::memory_order_relaxed);
    
    // Safety check: ensure pattern is valid
    if (pattern.lengthSteps == 0 || pattern.lengthSteps > MAX_STEPS) {
        return;
    }
    
    // Safety check: ensure current step is within valid range
    if (currentStep >= pattern.lengthSteps || currentStep >= MAX_STEPS) {
        return;
    }
    
    constexpr uint32_t MAX_TRIGGERS = 16;  // Match MAX_TRIGGERS_PER_CALLBACK
    
    // Evaluate each part
    for (uint32_t partIndex = 0; partIndex < NUM_PARTS; partIndex++) {
        // Safety check: prevent buffer overflow
        if (*numTriggers >= MAX_TRIGGERS) {
            break;  // Prevent buffer overflow
        }
        
        // Safety check: validate part index (defensive programming)
        if (partIndex >= NUM_PARTS) {
            break;
        }
        
        // Safety check: validate current step before array access
        if (currentStep >= pattern.lengthSteps || currentStep >= MAX_STEPS) {
            break;
        }
        
        StepTrigger& trigger = triggers[*numTriggers];
        trigger.partIndex = partIndex;
        trigger.stepIndex = currentStep;
        
        // Get step data (now safe due to bounds checks above)
        const StepData& step = pattern.steps[partIndex][currentStep];
        
        // Check gate
        if (step.gate == 0) {
            trigger.triggered = false;
            trigger.velocity = VELOCITY_NORMAL;
            continue;
        }
        
        // Evaluate probability (deterministic!)
        bool shouldStepTrigger = evaluateStepTrigger(pattern, partIndex, currentStep, loopIteration);
        
        if (shouldStepTrigger) {
            trigger.triggered = true;
            trigger.velocity = step.velocity;
            (*numTriggers)++;
        } else {
            trigger.triggered = false;
            trigger.velocity = VELOCITY_NORMAL;
        }
    }
    
    // Update previous step index
    mPreviousStepIndex.store(currentStep, std::memory_order_relaxed);
    mLastStepSampleCounter.store(sampleCounter, std::memory_order_relaxed);
}

bool Sequencer::evaluateStepTrigger(const Pattern& pattern, uint32_t partIndex, uint32_t stepIndex, uint32_t loopIteration) {
    // Safety check: validate part index
    if (partIndex >= NUM_PARTS) {
        return false;
    }
    
    // Safety check: validate pattern
    if (pattern.lengthSteps == 0 || pattern.lengthSteps > MAX_STEPS) {
        return false;
    }
    
    // Safety check: validate step index
    if (stepIndex >= pattern.lengthSteps || stepIndex >= MAX_STEPS) {
        return false;
    }
    
    // Get step data
    const StepData& step = pattern.steps[partIndex][stepIndex];
    
    // Evaluate probability (deterministic)
    return shouldTrigger(step.probability, pattern.patternSeed, stepIndex, loopIteration);
}

int64_t Sequencer::applyMicrotiming(int64_t sampleCounter, int8_t microtiming, double sampleRate) {
    // Apply microtiming offset in samples
    // Microtiming is in "ticks" (1 tick = 1 sample at current BPM)
    
    // Clamp microtiming to valid range
    int8_t clampedMicrotiming = std::max(MICROTIMING_MIN, std::min(MICROTIMING_MAX, microtiming));
    
    // Apply offset
    return sampleCounter + clampedMicrotiming;
}

void Sequencer::calculateCurrentStep(int64_t sampleCounter, double sampleRate, uint32_t patternLength) {
    // Safety check: prevent division by zero
    if (mSamplesPerStep <= 0.0) {
        // Invalid samples per step - keep current step
        return;
    }
    
    // Safety check: prevent division by zero for pattern length
    if (patternLength == 0) {
        // Invalid pattern length - reset to default
        mCurrentStepIndex.store(0, std::memory_order_relaxed);
        mLoopIteration.store(0, std::memory_order_relaxed);
        return;
    }
    
    // Calculate total steps elapsed
    int64_t totalSteps = static_cast<int64_t>(sampleCounter / mSamplesPerStep);
    
    // Calculate loop iteration
    uint32_t loopIteration = static_cast<uint32_t>(totalSteps / patternLength);
    mLoopIteration.store(loopIteration, std::memory_order_relaxed);
    
    // Calculate step index within pattern
    uint32_t stepIndex = static_cast<uint32_t>(totalSteps % patternLength);
    
    // Safety check: ensure step index is within valid range
    if (stepIndex >= patternLength) {
        stepIndex = patternLength - 1;
    }
    
    // Additional safety: clamp to MAX_STEPS
    if (stepIndex >= MAX_STEPS) {
        stepIndex = MAX_STEPS - 1;
    }
    
    mCurrentStepIndex.store(stepIndex, std::memory_order_relaxed);
}

bool Sequencer::hasStepChanged() const {
    uint32_t current = mCurrentStepIndex.load(std::memory_order_relaxed);
    uint32_t previous = mPreviousStepIndex.load(std::memory_order_relaxed);
    
    return current != previous;
}

} // namespace Tribex
