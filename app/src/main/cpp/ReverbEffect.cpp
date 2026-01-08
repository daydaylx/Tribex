#include "ReverbEffect.h"
#include <cstring>
#include <algorithm>

namespace Tribex {

ReverbEffect::ReverbEffect()
    : mNumActiveLines(MAX_LINES)
    , mSize(0.5f)
    , mDensity(0.5f)
    , mMix(0.3f)
    , mEnabled(true)
    , mCurrentMix(0.3f) {
    
    // Initialize reverb lines
    for (int32_t i = 0; i < MAX_LINES; ++i) {
        std::memset(mLines[i].buffer, 0, sizeof(mLines[i].buffer));
        mLines[i].writeHead = 0;
        mLines[i].delay = PRIME_DELAYS[i];
        mLines[i].filterState = 0.0f;
    }
}

void ReverbEffect::setSize(float size) {
    size = std::max(0.0f, std::min(1.0f, size));
    mSize.store(size);
}

void ReverbEffect::setDensity(float density) {
    density = std::max(0.0f, std::min(1.0f, density));
    mDensity.store(density);
}

void ReverbEffect::setMix(float mix) {
    mix = std::max(0.0f, std::min(1.0f, mix));
    mMix.store(mix);
}

void ReverbEffect::clear() {
    for (int32_t i = 0; i < MAX_LINES; ++i) {
        std::memset(mLines[i].buffer, 0, sizeof(mLines[i].buffer));
        mLines[i].filterState = 0.0f;
    }
}

void ReverbEffect::setEnabled(bool enabled) {
    mEnabled.store(enabled);
    if (!enabled) {
        clear();
    }
}

void ReverbEffect::setDensityLevel(int level) {
    switch (level) {
        case 0:  // High density
            mNumActiveLines = 8;
            break;
        case 1:  // Low density
            mNumActiveLines = 4;
            break;
        case 2:  // Off
            mNumActiveLines = 0;
            setEnabled(false);
            return;
    }
    // Re-enable if not level 2
    setEnabled(true);
}

void ReverbEffect::process(float* leftIn, float* rightIn, 
                           float* leftOut, float* rightOut, 
                           int32_t numFrames) {
    
    // Early exit if disabled
    if (!mEnabled.load() || mNumActiveLines == 0) {
        std::memcpy(leftOut, leftIn, numFrames * sizeof(float));
        std::memcpy(rightOut, rightIn, numFrames * sizeof(float));
        return;
    }

    // Smooth parameter changes
    const float smoothing = 0.005f;
    float mix = mMix.load();
    mCurrentMix += (mix - mCurrentMix) * smoothing;

    float size = mSize.load();
    float density = mDensity.load();
    float attenuation = 1.0f - (density * 0.5f);  // Higher density = less attenuation

    // Clear output buffers
    std::memset(leftOut, 0, numFrames * sizeof(float));
    std::memset(rightOut, 0, numFrames * sizeof(float));

    // Process each reverb line
    for (int32_t line = 0; line < mNumActiveLines; ++line) {
        ReverbLine& rline = mLines[line];
        int32_t bufferSize = 4096;
        int32_t mask = bufferSize - 1;

        // Apply size modulation to delay
        int32_t delay = rline.delay + static_cast<int32_t>(size * 100.0f);
        delay = std::min(delay, 2048);

        for (int32_t i = 0; i < numFrames; ++i) {
            // Calculate read position
            int32_t readHead = (rline.writeHead - delay + bufferSize) & mask;

            // Read delayed signal
            float delayed = rline.buffer[readHead];

            // Apply 1-pole lowpass filter (tames high frequencies)
            rline.filterState = delayed * 0.1f + rline.filterState * 0.9f;
            float filtered = rline.filterState;

            // Write to buffer (input + feedback)
            float monoInput = (leftIn[i] + rightIn[i]) * 0.5f;
            rline.buffer[rline.writeHead] = monoInput + filtered * attenuation;

            // Add to output (wet signal)
            float wet = filtered;
            leftOut[i] += wet;
            rightOut[i] += wet;

            // Advance write head
            rline.writeHead = (rline.writeHead + 1) & mask;
        }
    }

    // Normalize reverb output (prevent clipping with many lines)
    float normalization = 1.0f / static_cast<float>(mNumActiveLines + 1);

    // Mix wet and dry
    for (int32_t i = 0; i < numFrames; ++i) {
        float wet = (leftOut[i] + rightOut[i]) * 0.5f * normalization;
        leftOut[i] = leftIn[i] * (1.0f - mCurrentMix) + wet * mCurrentMix;
        rightOut[i] = rightIn[i] * (1.0f - mCurrentMix) + wet * mCurrentMix;
    }
}

} // namespace Tribex