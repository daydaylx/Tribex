#ifndef TRIBEX_LIMITER_H
#define TRIBEX_LIMITER_H

#include <array>
#include <atomic>
#include <cstdint>

namespace Tribex {

class Limiter {
public:
    Limiter();
    ~Limiter() = default;

    // Set parameters (thread-safe via atomics)
    void setThresholdDb(float thresholdDb); // -12.0 to -0.3 dB
    void setReleaseMs(float releaseMs);       // 10 to 1000 ms

    // Process audio (realtime, no allocations!)
    void process(float* leftIn, float* rightIn, float* leftOut, float* rightOut, int32_t numFrames);

    // Clear state
    void clear();

    // Enable/disable (limiter should usually be enabled)
    void setEnabled(bool enabled);
    bool isEnabled() const { return mEnabled.load(); }

private:
    // Convert dB to linear
    static float dbToLinear(float db);

    // Convert linear to dB
    static float linearToDb(float linear);

    // Lookahead buffer (1.5ms at 48kHz = 72 samples)
    static constexpr int32_t LOOKAHEAD_SAMPLES = 72;
    static constexpr int32_t LOOKAHEAD_MASK = LOOKAHEAD_SAMPLES - 1;
    std::array<float, LOOKAHEAD_SAMPLES> mLookaheadBufferLeft;
    std::array<float, LOOKAHEAD_SAMPLES> mLookaheadBufferRight;
    int32_t mWriteHead;

    // Gain reduction state
    float mGainReduction;
    float mPeakFollower;

    // Parameters (atomic for lock-free access)
    std::atomic<float> mThreshold;  // Linear (0.0 - 1.0)
    std::atomic<float> mRelease;    // Release time coefficient
    std::atomic<bool> mEnabled;

    // Hard ceiling (fixed at -0.3dB)
    static constexpr float CEILING_DB = -0.3f;
    static constexpr float CEILING_LINEAR = 0.966f;  // 10^(-0.3/20)

    // Soft knee parameter
    static constexpr float KNEE_DB = 6.0f;
    static constexpr float KNEE_LINEAR = 2.0f;  // 10^(6/20)
};

} // namespace Tribex

#endif // TRIBEX_LIMITER_H