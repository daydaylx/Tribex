#include "Sequencer.h"
#include <algorithm>

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
    mActivePattern = Pattern();
    mActiveChain = Chain();
}

void Sequencer::start() {
    mIsPlaying.store(true, std::memory_order_relaxed);
    
    // Reset step counter on start
    mLastStepSampleCounter.store(0, std::memory_order_relaxed);
    mCurrentStepIndex.store(0, std::memory_order_relaxed);
    mPreviousStepIndex.store(0, std::memory_order_relaxed);
    mLoopIteration.store(0, std::memory_order_relaxed);
}

void Sequencer::stop() {
    mIsPlaying.store(false, std::memory_order_relaxed);
}

void Sequencer::loadPattern(const Pattern& pattern) {
    mActivePattern = pattern;
    
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
    
    // Check if sequencer is playing
    if (!mIsPlaying.load(std::memory_order_relaxed)) {
        *numTriggers = 0;
        return;
    }
    
    // Update samples per step (in case BPM changed)
    mSamplesPerStep = calculateSamplesPerStep(sampleRate);
    
    // Calculate current step
    calculateCurrentStep(sampleCounter, sampleRate);
    
    // Check if step changed
    if (!hasStepChanged()) {
        *numTriggers = 0;
        return;
    }
    
    // Step changed - evaluate triggers for all parts
    uint32_t currentStep = mCurrentStepIndex.load(std::memory_order_relaxed);
    uint32_t loopIteration = mLoopIteration.load(std::memory_order_relaxed);
    
    *numTriggers = 0;
    
    // Evaluate each part
    for (uint32_t partIndex = 0; partIndex < NUM_PARTS; partIndex++) {
        StepTrigger& trigger = triggers[*numTriggers];
        trigger.partIndex = partIndex;
        trigger.stepIndex = currentStep;
        
        // Get step data
        const StepData& step = mActivePattern.steps[partIndex][currentStep];
        
        // Check gate
        if (step.gate == 0) {
            trigger.triggered = false;
            trigger.velocity = VELOCITY_NORMAL;
            continue;
        }
        
        // Evaluate probability (deterministic!)
        bool shouldStepTrigger = evaluateStepTrigger(partIndex, currentStep, loopIteration);
        
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

bool Sequencer::evaluateStepTrigger(uint32_t partIndex, uint32_t stepIndex, uint32_t loopIteration) {
    // Get step data
    const StepData& step = mActivePattern.steps[partIndex][stepIndex];
    
    // Evaluate probability (deterministic)
    return shouldTrigger(step.probability, mActivePattern.patternSeed, stepIndex, loopIteration);
}

int64_t Sequencer::applyMicrotiming(int64_t sampleCounter, int8_t microtiming, double sampleRate) {
    // Apply microtiming offset in samples
    // Microtiming is in "ticks" (1 tick = 1 sample at current BPM)
    
    // Clamp microtiming to valid range
    int8_t clampedMicrotiming = std::max(MICROTIMING_MIN, std::min(MICROTIMING_MAX, microtiming));
    
    // Apply offset
    return sampleCounter + clampedMicrotiming;
}

void Sequencer::calculateCurrentStep(int64_t sampleCounter, double sampleRate) {
    // Calculate total steps elapsed
    int64_t totalSteps = static_cast<int64_t>(sampleCounter / mSamplesPerStep);
    
    // Calculate loop iteration
    uint32_t loopIteration = static_cast<uint32_t>(totalSteps / mActivePattern.lengthSteps);
    mLoopIteration.store(loopIteration, std::memory_order_relaxed);
    
    // Calculate step index within pattern
    uint32_t stepIndex = static_cast<uint32_t>(totalSteps % mActivePattern.lengthSteps);
    mCurrentStepIndex.store(stepIndex, std::memory_order_relaxed);
}

bool Sequencer::hasStepChanged() const {
    uint32_t current = mCurrentStepIndex.load(std::memory_order_relaxed);
    uint32_t previous = mPreviousStepIndex.load(std::memory_order_relaxed);
    
    return current != previous;
}

} // namespace Tribex