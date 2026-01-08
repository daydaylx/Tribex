#ifndef TRIBEX_REVERBEFFECT_H
#define TRIBEX_REVERBEFFECT_H

#include <array>
#include <atomic>
#include <cstdint>

namespace Tribex {

class ReverbEffect {
public:
    ReverbEffect();
    ~ReverbEffect() = default;

    // Set parameters (thread-safe via atomics)
    void setSize(float size);       // 0.0 - 1.0 (Room size)
    void setDensity(float density); // 0.0 - 1.0
    void setMix(float mix);         // 0.0 - 1.0 (Dry/Wet)

    // Process audio (realtime, no allocations!)
    void process(float* leftIn, float* rightIn, float* leftOut, float* rightOut, int32_t numFrames);

    // Clear reverb buffer
    void clear();

    // Enable/disable (for degradation)
    void setEnabled(bool enabled);
    bool isEnabled() const { return mEnabled.load(); }

    // Set density level for performance degradation
    void setDensityLevel(int level);  // 0 = high, 1 = low, 2 = off

private:
    // Reverb line (single delay line)
    struct ReverbLine {
        float buffer[4096];  // Max ~85ms at 48kHz
        int32_t writeHead;
        int32_t delay;
        float filterState;  // Simple 1-pole lowpass filter
    };

    // Number of delay lines (variable for degradation)
    static constexpr int MAX_LINES = 8;
    int32_t mNumActiveLines;
    std::array<ReverbLine, MAX_LINES> mLines;

    // Prime number delays for natural reverb
    static constexpr int32_t PRIME_DELAYS[MAX_LINES] = {
        179, 211, 263, 331, 397, 467, 541, 619
    };

    // Parameters (atomic for lock-free access)
    std::atomic<float> mSize;
    std::atomic<float> mDensity;
    std::atomic<float> mMix;
    std::atomic<bool> mEnabled;

    // Smooth parameter changes
    float mCurrentMix;
};

} // namespace Tribex

#endif // TRIBEX_REVERBEFFECT_H