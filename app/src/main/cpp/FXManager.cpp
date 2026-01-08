#include "FXManager.h"
#include <cstring>
#include <cmath>

namespace Tribex {

FXManager::FXManager()
    : mDegradationLevel(DegradationLevel::LEVEL_0_OPTIMAL)
    , mXRunCount(0)
    , mLastXRunTime(0) {
    
    // Clear temp buffers
    mTempBuffer1Left.fill(0.0f);
    mTempBuffer1Right.fill(0.0f);
    mTempBuffer2Left.fill(0.0f);
    mTempBuffer2Right.fill(0.0f);
}

// Delay parameter control
void FXManager::setDelayTimeMs(float timeMs) {
    mDelay.setTimeMs(timeMs);
}

void FXManager::setDelayFeedback(float feedback) {
    mDelay.setFeedback(feedback);
}

void FXManager::setDelayMix(float mix) {
    mDelay.setMix(mix);
}

// Reverb parameter control
void FXManager::setReverbSize(float size) {
    mReverb.setSize(size);
}

void FXManager::setReverbDensity(float density) {
    mReverb.setDensity(density);
}

void FXManager::setReverbMix(float mix) {
    mReverb.setMix(mix);
}

// Valve parameter control
void FXManager::setValveAmount(float amount) {
    mValve.setAmount(amount);
}

// Limiter parameter control
void FXManager::setLimiterThresholdDb(float thresholdDb) {
    mLimiter.setThresholdDb(thresholdDb);
}

void FXManager::setLimiterReleaseMs(float releaseMs) {
    mLimiter.setReleaseMs(releaseMs);
}

void FXManager::process(float* leftIn, float* rightIn,
                          float* leftOut, float* rightOut,
                          int32_t numFrames) {
    // Safety check: null pointer validation
    if (!leftIn || !rightIn || !leftOut || !rightOut) {
        return;
    }
    
    // Safety check: validate numFrames
    if (numFrames <= 0) {
        return;
    }
    
    // Ensure buffer size
    if (numFrames > MAX_FRAMES_PER_CALLBACK) {
        // Should not happen, but fallback to direct pass-through
        // Safety: clamp to MAX_FRAMES_PER_CALLBACK
        int32_t safeNumFrames = MAX_FRAMES_PER_CALLBACK;
        std::memcpy(leftOut, leftIn, safeNumFrames * sizeof(float));
        std::memcpy(rightOut, rightIn, safeNumFrames * sizeof(float));
        return;
    }

    // FX Chain: Delay → Reverb → Valve → Limiter
    
    // Step 1: Delay (input → temp1)
    mDelay.process(leftIn, rightIn, mTempBuffer1Left.data(), mTempBuffer1Right.data(), numFrames);
    
    // Step 2: Reverb (temp1 → temp2)
    mReverb.process(mTempBuffer1Left.data(), mTempBuffer1Right.data(),
                   mTempBuffer2Left.data(), mTempBuffer2Right.data(), numFrames);
    
    // Step 3: Valve (temp2 → temp1)
    mValve.process(mTempBuffer2Left.data(), mTempBuffer2Right.data(),
                   mTempBuffer1Left.data(), mTempBuffer1Right.data(), numFrames);
    
    // Step 4: Limiter (temp1 → output)
    mLimiter.process(mTempBuffer1Left.data(), mTempBuffer1Right.data(),
                    leftOut, rightOut, numFrames);
}

void FXManager::clearAll() {
    mDelay.clear();
    mReverb.clear();
    mLimiter.clear();
}

void FXManager::setDegradationLevel(DegradationLevel level) {
    mDegradationLevel.store(level);
    applyDegradation(level);
}

void FXManager::reportXRun() {
    mXRunCount.fetch_add(1, std::memory_order_relaxed);
    mLastXRunTime = getCurrentTimeMs();
    
    // Check if we need to upgrade degradation level
    DegradationLevel current = mDegradationLevel.load();
    if (current == DegradationLevel::LEVEL_0_OPTIMAL && mXRunCount.load() > 10) {
        setDegradationLevel(DegradationLevel::LEVEL_1_WARNING);
    } else if (current == DegradationLevel::LEVEL_1_WARNING && mXRunCount.load() > 20) {
        setDegradationLevel(DegradationLevel::LEVEL_2_CRITICAL);
    }
}

void FXManager::resetXRunCounter() {
    mXRunCount.store(0);
    
    // Try to downgrade degradation level if audio is stable
    DegradationLevel current = mDegradationLevel.load();
    if (current == DegradationLevel::LEVEL_2_CRITICAL) {
        setDegradationLevel(DegradationLevel::LEVEL_1_WARNING);
    } else if (current == DegradationLevel::LEVEL_1_WARNING) {
        setDegradationLevel(DegradationLevel::LEVEL_0_OPTIMAL);
    }
}

int32_t FXManager::getMaxVoices() const {
    DegradationLevel level = mDegradationLevel.load();
    switch (level) {
        case DegradationLevel::LEVEL_0_OPTIMAL:
            return 24;  // Max voices
        case DegradationLevel::LEVEL_1_WARNING:
            return 16;  // -33%
        case DegradationLevel::LEVEL_2_CRITICAL:
            return 8;   // -67%
    }
    return 24;  // Default
}

void FXManager::applyDegradation(DegradationLevel level) {
    switch (level) {
        case DegradationLevel::LEVEL_0_OPTIMAL:
            // Max quality
            mDelay.setEnabled(true);
            mReverb.setEnabled(true);
            mReverb.setDensityLevel(0);  // High density
            mValve.setEnabled(true);
            mLimiter.setEnabled(true);
            break;
            
        case DegradationLevel::LEVEL_1_WARNING:
            // Reduced quality
            mDelay.setEnabled(true);
            mReverb.setEnabled(true);
            mReverb.setDensityLevel(1);  // Low density
            mValve.setEnabled(true);
            mLimiter.setEnabled(true);
            break;
            
        case DegradationLevel::LEVEL_2_CRITICAL:
            // Minimum quality
            mDelay.setEnabled(true);
            mReverb.setEnabled(false);  // Reverb OFF
            mValve.setEnabled(false);  // Valve bypass
            mLimiter.setEnabled(true);   // Limiter always active
            break;
    }
}

int64_t FXManager::getCurrentTimeMs() {
    // Simple time tracking (for XRun window)
    // In production, this should use a proper clock
    // For now, use sample counter approximation
    static int64_t sampleCounter = 0;
    sampleCounter += 1000;  // Approximate increment
    return sampleCounter;
}

} // namespace Tribex