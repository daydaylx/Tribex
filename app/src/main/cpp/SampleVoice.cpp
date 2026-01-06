#include "SampleVoice.h"
#include <algorithm>
#include <cmath>

namespace Tribex {

SampleVoice::SampleVoice()
    : mState(VoiceState::IDLE)
    , mPartIndex(0)
    , mSampleData(nullptr)
    , mSampleLength(0)
    , mSampleRate(44100)
    , mPosition(0.0f)
    , mPitch(0.0f)
    , mPan(0.0f)
    , mLevel(1.0f)
    , mDecayMs(200.0f)
    , mFilterType(FilterType::LP)
    , mDecayEnvelope(1.0f)
    , mDecayPhase(0.0f)
    , mFilterX1(0.0f)
    , mFilterX2(0.0f)
    , mFilterY1(0.0f)
    , mFilterY2(0.0f)
    , mVelocity(1.0f)
{
}

SampleVoice::~SampleVoice() {
    // Sample data is not owned by voice
}

void SampleVoice::start(const float* sampleData, uint32_t length, uint32_t sampleRate,
                      float velocity) {
    // Store sample data (not owned by voice)
    mSampleData = sampleData;
    mSampleLength = length;
    mSampleRate = sampleRate;
    
    // Store velocity for level scaling
    mVelocity = velocity;
    
    // Reset playback position
    mPosition = 0.0f;
    
    // Reset decay envelope
    mDecayEnvelope = 1.0f;
    mDecayPhase = 0.0f;
    
    // Reset filter state
    mFilterX1 = 0.0f;
    mFilterX2 = 0.0f;
    mFilterY1 = 0.0f;
    mFilterY2 = 0.0f;
    
    // Set state to playing
    mState.store(VoiceState::PLAYING, std::memory_order_release);
}

void SampleVoice::stop() {
    mState.store(VoiceState::IDLE, std::memory_order_release);
}

float SampleVoice::calculatePitchRatio(float semitones) const {
    // 2^(semitones / 12)
    return std::pow(2.0f, semitones / 12.0f);
}

float SampleVoice::applyDecay(float sample, float position, float totalLength) {
    // Calculate decay phase (0.0 to 1.0)
    float phase = position / totalLength;
    
    // Convert decayMs to phase fraction
    // If decayMs is 200ms and sample is 500ms long, decay phase is 0.4
    float decayPhaseFraction = mDecayMs.load(std::memory_order_relaxed) / 1000.0f;
    float lengthMs = (totalLength / static_cast<float>(mSampleRate)) * 1000.0f;
    float decayEndPhase = decayPhaseFraction / (lengthMs / 1000.0f);
    
    // If decay phase is longer than sample, no decay
    if (decayEndPhase >= 1.0f) {
        return sample;
    }
    
    // Apply exponential decay from decayEndPhase to end
    if (phase < decayEndPhase) {
        return sample;  // No decay yet
    }
    
    // Exponential decay
    float decayPhase = (phase - decayEndPhase) / (1.0f - decayEndPhase);
    return sample * std::exp(-3.0f * decayPhase);  // -60dB at end
}

float SampleVoice::applyFilter(float sample) {
    // Simple DJ-style filter using biquad
    // This is a basic implementation - could be improved for M5
    
    FilterType filterType = mFilterType.load(std::memory_order_relaxed);
    
    // For M4, we'll implement a simple LPF and HPF
    // More sophisticated filters can be added in M5
    
    if (filterType == FilterType::LP) {
        // Simple 1-pole LPF (no resonance for M4)
        float alpha = 0.1f;  // Cutoff frequency
        float output = mFilterY1 + alpha * (sample - mFilterY1);
        mFilterY1 = output;
        return output;
    } else {
        // Simple 1-pole HPF
        float alpha = 0.1f;
        float output = alpha * (sample - mFilterX1) + (1.0f - alpha) * mFilterY1;
        mFilterX1 = sample;
        mFilterY1 = output;
        return output;
    }
}

void SampleVoice::advancePosition(float pitchRatio) {
    mPosition += pitchRatio;
}

void SampleVoice::renderSample(float& left, float& right) {
    VoiceState state = mState.load(std::memory_order_acquire);
    
    if (state == VoiceState::IDLE) {
        left = 0.0f;
        right = 0.0f;
        return;
    }
    
    if (mSampleData == nullptr || mSampleLength == 0) {
        left = 0.0f;
        right = 0.0f;
        mState.store(VoiceState::IDLE, std::memory_order_release);
        return;
    }
    
    // Calculate pitch ratio
    float pitchSemitones = mPitch.load(std::memory_order_relaxed);
    float pitchRatio = calculatePitchRatio(pitchSemitones);
    
    // Linear interpolation resampling
    uint32_t index0 = static_cast<uint32_t>(mPosition);
    uint32_t index1 = index0 + 1;
    float frac = mPosition - index0;
    
    // Check bounds
    if (index0 >= mSampleLength) {
        mState.store(VoiceState::IDLE, std::memory_order_release);
        left = 0.0f;
        right = 0.0f;
        return;
    }
    
    // Get samples (with bound check for index1)
    float sample0 = mSampleData[index0];
    float sample1 = (index1 < mSampleLength) ? mSampleData[index1] : sample0;
    
    // Linear interpolation
    float sample = sample0 + (sample1 - sample0) * frac;
    
    // Apply decay envelope
    sample = applyDecay(sample, mPosition, static_cast<float>(mSampleLength));
    
    // Apply filter
    sample = applyFilter(sample);
    
    // Get parameters
    float pan = mPan.load(std::memory_order_relaxed);
    float level = mLevel.load(std::memory_order_relaxed);
    
    // Apply level with velocity
    sample *= level * mVelocity;
    
    // Clamp to prevent clipping
    sample = std::max(-1.0f, std::min(1.0f, sample));
    
    // Stereo panning (constant power)
    float leftGain = std::sqrt(0.5f * (1.0f + pan));
    float rightGain = std::sqrt(0.5f * (1.0f - pan));
    
    left = sample * leftGain;
    right = sample * rightGain;
    
    // Advance position
    advancePosition(pitchRatio);
    
    // Check if we've reached end of sample
    if (index0 >= mSampleLength - 1) {
        mState.store(VoiceState::IDLE, std::memory_order_release);
    }
}

} // namespace Tribex