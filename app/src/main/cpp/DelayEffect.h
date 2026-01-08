#ifndef TRIBEX_DELAYEFFECT_H
#define TRIBEX_DELAYEFFECT_H

#include <array>
#include <atomic>
#include <cstdint>
#include <cmath>

namespace Tribex {

class DelayEffect {
public:
    DelayEffect();
    ~DelayEffect() = default;

    // Set parameters (thread-safe via atomics)
    void setTimeMs(float timeMs);  // 0 - 1000ms
    void setFeedback(float feedback); // 0.0 - 0.95
    void setMix(float mix);          // 0.0 - 1.0 (Dry/Wet)

    // Process audio (realtime, no allocations!)
    void process(float* leftIn, float* rightIn, float* leftOut, float* rightOut, int32_t numFrames);

    // Clear delay buffer
    void clear();

    // Enable/disable (for degradation)
    void setEnabled(bool enabled);
    bool isEnabled() const { return mEnabled.load(); }

private:
    // Delay buffer (preallocated, max 2 seconds at 48kHz)
    static constexpr int32_t MAX_DELAY_SAMPLES = 96000;  // 2000ms at 48kHz
    static constexpr int32_t WRITE_HEAD_MASK = MAX_DELAY_SAMPLES - 1;
    std::array<float, MAX_DELAY_SAMPLES> mDelayBufferLeft;
    std::array<float, MAX_DELAY_SAMPLES> mDelayBufferRight;

    // Write head (circular buffer)
    int32_t mWriteHead;

    // Parameters (atomic for lock-free access)
    std::atomic<float> mDelaySamples;  // Delay in samples
    std::atomic<float> mFeedback;
    std::atomic<float> mMix;
    std::atomic<bool> mEnabled;

    // Smooth parameter changes (linear interpolation)
    float mCurrentFeedback;
    float mCurrentMix;
};

} // namespace Tribex

#endif // TRIBEX_DELAYEFFECT_H