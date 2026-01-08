#ifndef TRIBEX_VALVESATURATION_H
#define TRIBEX_VALVESATURATION_H

#include <atomic>
#include <cstdint>

namespace Tribex {

class ValveSaturation {
public:
    ValveSaturation();
    ~ValveSaturation() = default;

    // Set parameters (thread-safe via atomics)
    void setAmount(float amount); // 0.0 - 1.0 (saturation amount)

    // Process audio (realtime, no allocations!)
    void process(float* leftIn, float* rightIn, float* leftOut, float* rightOut, int32_t numFrames);

    // Enable/disable (for degradation)
    void setEnabled(bool enabled);
    bool isEnabled() const { return mEnabled.load(); }

private:
    // Asymmetric waveshaper function (tanh-like approximation)
    static float waveshaper(float x, float amount);

    // Pre-emphasis EQ (highpass)
    void preEmphasis(float input, float& out, float& state);

    // De-emphasis EQ (lowpass)
    void deEmphasis(float input, float& out, float& state);

    // Parameters (atomic for lock-free access)
    std::atomic<float> mAmount;
    std::atomic<bool> mEnabled;

    // Filter states for pre/de-emphasis
    float mPreLeftState;
    float mPreRightState;
    float mDeLeftState;
    float mDeRightState;

    // Smooth parameter changes
    float mCurrentAmount;
};

} // namespace Tribex

#endif // TRIBEX_VALVESATURATION_H