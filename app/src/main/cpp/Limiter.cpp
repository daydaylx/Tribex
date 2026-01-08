#include "Limiter.h"
#include <cstring>
#include <cmath>

namespace Tribex {

Limiter::Limiter()
    : mWriteHead(0)
    , mGainReduction(1.0f)
    , mPeakFollower(0.0f)
    , mThreshold(CEILING_LINEAR)
    , mRelease(0.1f)
    , mEnabled(true) {
    
    // Clear lookahead buffers
    mLookaheadBufferLeft.fill(0.0f);
    mLookaheadBufferRight.fill(0.0f);
}

float Limiter::dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

float Limiter::linearToDb(float linear) {
    return 20.0f * std::log10(std::max(0.0001f, linear));
}

void Limiter::setThresholdDb(float thresholdDb) {
    // Clamp to valid range
    thresholdDb = std::max(-12.0f, std::min(CEILING_DB, thresholdDb));
    float linear = dbToLinear(thresholdDb);
    mThreshold.store(linear);
}

void Limiter::setReleaseMs(float releaseMs) {
    // Clamp to valid range
    releaseMs = std::max(10.0f, std::min(1000.0f, releaseMs));
    
    // Convert to release coefficient (exp(-dt/release))
    const float sampleRate = 48000.0f;
    const float dt = 1.0f / sampleRate;
    float releaseSec = releaseMs / 1000.0f;
    mRelease.store(std::exp(-dt / releaseSec));
}

void Limiter::clear() {
    mLookaheadBufferLeft.fill(0.0f);
    mLookaheadBufferRight.fill(0.0f);
    mGainReduction = 1.0f;
    mPeakFollower = 0.0f;
    mWriteHead = 0;
}

void Limiter::setEnabled(bool enabled) {
    mEnabled.store(enabled);
    if (!enabled) {
        clear();
    }
}

void Limiter::process(float* leftIn, float* rightIn,
                       float* leftOut, float* rightOut,
                       int32_t numFrames) {
    
    // Early exit if disabled
    if (!mEnabled.load()) {
        std::memcpy(leftOut, leftIn, numFrames * sizeof(float));
        std::memcpy(rightOut, rightIn, numFrames * sizeof(float));
        return;
    }

    float threshold = mThreshold.load();
    float releaseCoeff = mRelease.load();

    for (int32_t i = 0; i < numFrames; ++i) {
        // Write input to lookahead buffer
        mLookaheadBufferLeft[mWriteHead] = leftIn[i];
        mLookaheadBufferRight[mWriteHead] = rightIn[i];

        // Calculate read position (lookahead delay)
        int32_t readHead = (mWriteHead - LOOKAHEAD_SAMPLES + LOOKAHEAD_SAMPLES * 2) & LOOKAHEAD_MASK;

        // Get sample from lookahead buffer (delayed by 1.5ms)
        float delayedLeft = mLookaheadBufferLeft[readHead];
        float delayedRight = mLookaheadBufferRight[readHead];

        // Calculate peak amplitude
        float peak = std::max(std::abs(delayedLeft), std::abs(delayedRight));

        // Smooth peak follower (attack is instant, release is controlled)
        if (peak > mPeakFollower) {
            mPeakFollower = peak;  // Instant attack
        } else {
            mPeakFollower = peak + (mPeakFollower - peak) * releaseCoeff;
        }

        // Calculate required gain reduction
        float targetGainReduction = 1.0f;
        
        if (mPeakFollower > threshold) {
            // Soft knee calculation
            float kneeRatio = (mPeakFollower - threshold) / (KNEE_LINEAR * threshold);
            
            if (kneeRatio < 1.0f) {
                // Inside soft knee region
                float kneeDepth = 1.0f - kneeRatio;
                float compressedRatio = threshold / mPeakFollower;
                targetGainReduction = kneeDepth + compressedRatio * kneeRatio;
            } else {
                // Hard limiting region
                targetGainReduction = threshold / mPeakFollower;
            }
        }

        // Smooth gain reduction (fast attack, controlled release)
        if (targetGainReduction < mGainReduction) {
            // Fast attack (5ms)
            const float attackCoeff = 0.78f;  // exp(-dt/5ms) at 48kHz
            mGainReduction = targetGainReduction + (mGainReduction - targetGainReduction) * attackCoeff;
        } else {
            // Release (controlled by parameter)
            mGainReduction = targetGainReduction + (mGainReduction - targetGainReduction) * releaseCoeff;
        }

        // Apply gain reduction to delayed samples
        leftOut[i] = delayedLeft * mGainReduction;
        rightOut[i] = delayedRight * mGainReduction;

        // Hard ceiling at -0.3dB
        if (leftOut[i] > CEILING_LINEAR) leftOut[i] = CEILING_LINEAR;
        if (leftOut[i] < -CEILING_LINEAR) leftOut[i] = -CEILING_LINEAR;
        if (rightOut[i] > CEILING_LINEAR) rightOut[i] = CEILING_LINEAR;
        if (rightOut[i] < -CEILING_LINEAR) rightOut[i] = -CEILING_LINEAR;

        // Advance write head
        mWriteHead = (mWriteHead + 1) & LOOKAHEAD_MASK;
    }
}

} // namespace Tribex