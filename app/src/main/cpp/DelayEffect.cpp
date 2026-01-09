#include "DelayEffect.h"
#include <cstring>
#include <algorithm>

namespace Tribex {

DelayEffect::DelayEffect()
    : mWriteHead(0)
    , mDelaySamples(0.0f)
    , mDelayTimeMs(0.0f)
    , mSampleRate(48000.0f)
    , mFeedback(0.0f)
    , mMix(0.0f)
    , mEnabled(true)
    , mCurrentFeedback(0.0f)
    , mCurrentMix(0.0f) {
    
    // Clear delay buffers
    mDelayBufferLeft.fill(0.0f);
    mDelayBufferRight.fill(0.0f);
}

void DelayEffect::setTimeMs(float timeMs) {
    // Clamp to valid range
    timeMs = std::max(0.0f, std::min(1000.0f, timeMs));
    
    mDelayTimeMs.store(timeMs);
    
    // Convert to samples using current sample rate
    float sampleRate = mSampleRate.load(std::memory_order_relaxed);
    if (sampleRate <= 0.0f) {
        sampleRate = 48000.0f;
    }
    float samples = timeMs * sampleRate / 1000.0f;
    
    // Clamp to buffer size
    samples = std::min(static_cast<float>(MAX_DELAY_SAMPLES), samples);
    mDelaySamples.store(samples);
}

void DelayEffect::setFeedback(float feedback) {
    // Clamp feedback to avoid instability
    feedback = std::max(0.0f, std::min(0.95f, feedback));
    mFeedback.store(feedback);
}

void DelayEffect::setMix(float mix) {
    // Clamp to valid range
    mix = std::max(0.0f, std::min(1.0f, mix));
    mMix.store(mix);
}

void DelayEffect::setSampleRate(float sampleRate) {
    if (sampleRate <= 0.0f) {
        sampleRate = 48000.0f;
    }
    mSampleRate.store(sampleRate, std::memory_order_relaxed);
    
    float timeMs = mDelayTimeMs.load(std::memory_order_relaxed);
    float samples = timeMs * sampleRate / 1000.0f;
    samples = std::min(static_cast<float>(MAX_DELAY_SAMPLES), samples);
    mDelaySamples.store(samples);
}

void DelayEffect::clear() {
    mDelayBufferLeft.fill(0.0f);
    mDelayBufferRight.fill(0.0f);
    mWriteHead = 0;
}

void DelayEffect::setEnabled(bool enabled) {
    mEnabled.store(enabled);
    if (!enabled) {
        // Clear buffer when disabled to prevent old audio
        clear();
    }
}

void DelayEffect::process(float* leftIn, float* rightIn, 
                         float* leftOut, float* rightOut, 
                         int32_t numFrames) {
    
    // Early exit if disabled
    if (!mEnabled.load()) {
        // Pass through
        std::memcpy(leftOut, leftIn, numFrames * sizeof(float));
        std::memcpy(rightOut, rightIn, numFrames * sizeof(float));
        return;
    }

    // Smooth parameter changes (10ms smoothing at 48kHz)
    const float smoothing = 0.005f;
    float feedback = mFeedback.load();
    float mix = mMix.load();
    
    mCurrentFeedback += (feedback - mCurrentFeedback) * smoothing;
    mCurrentMix += (mix - mCurrentMix) * smoothing;

    float delaySamples = mDelaySamples.load();
    int32_t delayInt = static_cast<int32_t>(delaySamples);

    for (int32_t i = 0; i < numFrames; ++i) {
        // Calculate read position (circular buffer)
        int32_t readHead = mWriteHead - delayInt;
        if (readHead < 0) {
            readHead += MAX_DELAY_SAMPLES;
        }

        // Get delayed signal
        float delayedLeft = mDelayBufferLeft[readHead];
        float delayedRight = mDelayBufferRight[readHead];

        // Apply feedback
        float feedbackLeft = delayedLeft * mCurrentFeedback;
        float feedbackRight = delayedRight * mCurrentFeedback;

        // Write to delay buffer (input + feedback)
        mDelayBufferLeft[mWriteHead] = leftIn[i] + feedbackLeft;
        mDelayBufferRight[mWriteHead] = rightIn[i] + feedbackRight;

        // Mix wet and dry
        float dry = leftIn[i];
        float wet = delayedLeft;
        leftOut[i] = dry * (1.0f - mCurrentMix) + wet * mCurrentMix;

        dry = rightIn[i];
        wet = delayedRight;
        rightOut[i] = dry * (1.0f - mCurrentMix) + wet * mCurrentMix;

        // Advance write head
        mWriteHead++;
        if (mWriteHead >= MAX_DELAY_SAMPLES) {
            mWriteHead = 0;
        }
    }
}

} // namespace Tribex
