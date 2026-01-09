#ifndef TRIBEX_FXMANAGER_H
#define TRIBEX_FXMANAGER_H

#include "DelayEffect.h"
#include "ReverbEffect.h"
#include "ValveSaturation.h"
#include "Limiter.h"
#include <array>
#include <atomic>
#include <cstdint>

namespace Tribex {

// Performance degradation levels
enum class DegradationLevel {
    LEVEL_0_OPTIMAL = 0,  // Max 24 voices, reverb high, valve active
    LEVEL_1_WARNING = 1,  // Max 16 voices, reverb low, valve active
    LEVEL_2_CRITICAL = 2  // Max 8 voices, reverb off, valve bypass
};

class FXManager {
public:
    FXManager();
    ~FXManager() = default;

    // FX Parameter control (thread-safe via atomics)
    
    // Delay
    void setDelayTimeMs(float timeMs);
    void setDelayFeedback(float feedback);
    void setDelayMix(float mix);
    void setSampleRate(float sampleRate);
    
    // Reverb
    void setReverbSize(float size);
    void setReverbDensity(float density);
    void setReverbMix(float mix);
    
    // Valve Saturation
    void setValveAmount(float amount);
    
    // Limiter
    void setLimiterThresholdDb(float thresholdDb);
    void setLimiterReleaseMs(float releaseMs);

    // Process audio through FX chain (realtime, no allocations!)
    // Chain: Delay → Reverb → Valve → Limiter
    void process(float* leftIn, float* rightIn, float* leftOut, float* rightOut, int32_t numFrames);

    // Clear all FX buffers
    void clearAll();

    // Performance degradation control
    void setDegradationLevel(DegradationLevel level);
    DegradationLevel getDegradationLevel() const { return mDegradationLevel.load(); }
    
    // XRun tracking for degradation detection
    void reportXRun();
    int32_t getXRunCount() const { return mXRunCount.load(); }
    void resetXRunCounter();

    // Get max voices for current degradation level
    int32_t getMaxVoices() const;

private:
    // FX instances
    DelayEffect mDelay;
    ReverbEffect mReverb;
    ValveSaturation mValve;
    Limiter mLimiter;

    // Preallocated buffers for FX chain (no allocations in process!)
    static constexpr int32_t MAX_FRAMES_PER_CALLBACK = 1024;
    std::array<float, MAX_FRAMES_PER_CALLBACK> mTempBuffer1Left;
    std::array<float, MAX_FRAMES_PER_CALLBACK> mTempBuffer1Right;
    std::array<float, MAX_FRAMES_PER_CALLBACK> mTempBuffer2Left;
    std::array<float, MAX_FRAMES_PER_CALLBACK> mTempBuffer2Right;

    // Degradation level
    std::atomic<DegradationLevel> mDegradationLevel;

    // XRun tracking
    std::atomic<int32_t> mXRunCount;
    int64_t mLastXRunTime;

    // Apply degradation settings to FX
    void applyDegradation(DegradationLevel level);
    
    // Get current time in milliseconds (for XRun window)
    int64_t getCurrentTimeMs();
};

} // namespace Tribex

#endif // TRIBEX_FXMANAGER_H
