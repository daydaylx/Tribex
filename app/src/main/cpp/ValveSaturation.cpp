#include "ValveSaturation.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace Tribex {

ValveSaturation::ValveSaturation()
    : mAmount(0.3f)
    , mEnabled(true)
    , mPreLeftState(0.0f)
    , mPreRightState(0.0f)
    , mDeLeftState(0.0f)
    , mDeRightState(0.0f)
    , mCurrentAmount(0.3f) {
}

float ValveSaturation::waveshaper(float x, float amount) {
    // Asymmetric waveshaper (simulates valve harmonics)
    // More saturation on positive side than negative
    
    // Soft clipping with asymmetric curve
    if (x > 0.0f) {
        // Positive side: more aggressive saturation
        float positiveLimit = 1.0f + amount * 0.5f;
        return std::tanh(x * positiveLimit) / positiveLimit;
    } else {
        // Negative side: milder saturation
        float negativeLimit = 1.0f - amount * 0.2f;
        return std::tanh(x * negativeLimit) / negativeLimit;
    }
}

void ValveSaturation::preEmphasis(float input, float& out, float& state) {
    // Highpass filter (boosts high frequencies before waveshaping)
    const float fc = 2000.0f;      // Cutoff frequency
    const float sampleRate = 48000.0f;
    const float RC = 1.0f / (2.0f * M_PI * fc);
    const float dt = 1.0f / sampleRate;
    const float alpha = RC / (RC + dt);
    
    // Highpass: out = input - state
    // State update: state = alpha * state + alpha * input
    out = input - state;
    state = alpha * state + alpha * input;
}

void ValveSaturation::deEmphasis(float input, float& out, float& state) {
    // Lowpass filter (restores frequency balance after waveshaping)
    const float fc = 2000.0f;      // Cutoff frequency (same as pre)
    const float sampleRate = 48000.0f;
    const float RC = 1.0f / (2.0f * M_PI * fc);
    const float dt = 1.0f / sampleRate;
    const float alpha = dt / (RC + dt);
    
    // Lowpass: out = alpha * input + (1.0f - alpha) * state
    out = alpha * input + (1.0f - alpha) * state;
    state = out;
}

void ValveSaturation::setAmount(float amount) {
    amount = std::max(0.0f, std::min(1.0f, amount));
    mAmount.store(amount);
}

void ValveSaturation::setEnabled(bool enabled) {
    mEnabled.store(enabled);
    // Reset filter states when disabling
    if (!enabled) {
        mPreLeftState = 0.0f;
        mPreRightState = 0.0f;
        mDeLeftState = 0.0f;
        mDeRightState = 0.0f;
    }
}

void ValveSaturation::process(float* leftIn, float* rightIn,
                             float* leftOut, float* rightOut,
                             int32_t numFrames) {
    
    // Early exit if disabled
    if (!mEnabled.load()) {
        std::memcpy(leftOut, leftIn, numFrames * sizeof(float));
        std::memcpy(rightOut, rightIn, numFrames * sizeof(float));
        return;
    }

    // Smooth parameter changes (10ms smoothing at 48kHz)
    const float smoothing = 0.005f;
    float amount = mAmount.load();
    mCurrentAmount += (amount - mCurrentAmount) * smoothing;

    for (int32_t i = 0; i < numFrames; ++i) {
        // Left channel
        float leftPre;
        preEmphasis(leftIn[i], leftPre, mPreLeftState);
        
        // Blend dry with saturated signal (amount controls wet/dry mix)
        float leftDry = leftIn[i] * (1.0f - mCurrentAmount);
        float leftWet = waveshaper(leftPre, mCurrentAmount * 3.0f) * mCurrentAmount;
        
        deEmphasis(leftDry + leftWet, leftOut[i], mDeLeftState);

        // Right channel
        float rightPre;
        preEmphasis(rightIn[i], rightPre, mPreRightState);
        
        float rightDry = rightIn[i] * (1.0f - mCurrentAmount);
        float rightWet = waveshaper(rightPre, mCurrentAmount * 3.0f) * mCurrentAmount;
        
        deEmphasis(rightDry + rightWet, rightOut[i], mDeRightState);
    }
}

} // namespace Tribex