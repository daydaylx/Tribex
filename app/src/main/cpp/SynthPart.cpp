#include "SynthPart.h"
#include <cmath>

namespace Tribex {

// NaN constant for "no lock"
constexpr float NO_LOCK = std::numeric_limits<float>::quiet_NaN();

SynthPart::SynthPart()
    : mWavetable(WavetableType::SAW)
    , mPitchSemitones(0.0f)
    , mCutoff(0.8f)
    , mResonance(0.0f)
    , mAmplitude(1.0f)
    , mAttack(10.0f)      // 10ms
    , mDecay(200.0f)     // 200ms
    , mSustain(0.7f)      // 70%
    , mRelease(300.0f)    // 300ms
    , mMuted(false)
    , mSoloed(false)
{
    // Voice will be initialized with defaults
}

SynthPart::~SynthPart() {
    stop();
}

void SynthPart::initialize(float sampleRate) {
    // Initialize wavetables
    WavetableSynthVoice::setSampleRate(sampleRate);
    WavetableSynthVoice::precomputeWavetables();
    
    // Set global parameters on voice
    mVoice.setWavetable(mWavetable);
    mVoice.setPitch(mPitchSemitones);
    mVoice.setCutoff(mCutoff);
    mVoice.setResonance(mResonance);
    mVoice.setAmplitude(mAmplitude);
    mVoice.setAttack(mAttack);
    mVoice.setDecay(mDecay);
    mVoice.setSustain(mSustain);
    mVoice.setRelease(mRelease);
}

void SynthPart::trigger(float velocity) {
    // Check mute state (M5: UI only, no audio effect yet)
    if (mMuted.load(std::memory_order_acquire)) {
        return;
    }
    
    // Apply current amplitude (from velocity + global amplitude)
    float amplitude = velocity * mAmplitude;
    
    // Trigger voice
    mVoice.start(amplitude);
}

void SynthPart::render(float* leftBuffer, float* rightBuffer, int32_t numFrames) {
    // Check mute state (M5: UI only, no audio effect yet)
    if (mMuted.load(std::memory_order_acquire)) {
        return;
    }
    
    // Render voice and sum to output buffers
    for (int32_t frame = 0; frame < numFrames; frame++) {
        float left, right;
        mVoice.renderSample(left, right);
        
        // Sum to output
        leftBuffer[frame] += left;
        rightBuffer[frame] += right;
    }
}

void SynthPart::stop() {
    mVoice.stop();
}

void SynthPart::setWavetable(WavetableType type) {
    mWavetable = type;
    mVoice.setWavetable(type);
}

void SynthPart::setPitch(float semitones) {
    mPitchSemitones = semitones;
    mVoice.setPitch(semitones);
}

void SynthPart::setCutoff(float normalizedCutoff) {
    mCutoff = normalizedCutoff;
    mVoice.setCutoff(normalizedCutoff);
}

void SynthPart::setResonance(float resonance) {
    mResonance = resonance;
    mVoice.setResonance(resonance);
}

void SynthPart::setAmplitude(float amplitude) {
    mAmplitude = amplitude;
    mVoice.setAmplitude(amplitude);
}

void SynthPart::setAttack(float attackMs) {
    mAttack = attackMs;
    mVoice.setAttack(attackMs);
}

void SynthPart::setDecay(float decayMs) {
    mDecay = decayMs;
    mVoice.setDecay(decayMs);
}

void SynthPart::setSustain(float sustainLevel) {
    mSustain = sustainLevel;
    mVoice.setSustain(sustainLevel);
}

void SynthPart::setRelease(float releaseMs) {
    mRelease = releaseMs;
    mVoice.setRelease(releaseMs);
}

/**
 * Apply step lock values
 * 
 * For each parameter, if lock is valid (not NaN), apply lock value.
 * Otherwise, apply global parameter value.
 */
void SynthPart::applyStepLocks(float pitchLock, float cutoffLock, float resonanceLock,
                                float amplitudeLock, float attackLock, float decayLock,
                                float sustainLock, float releaseLock) {
    // Pitch (semitones)
    if (!std::isnan(pitchLock)) {
        mVoice.setPitch(pitchLock);
    } else {
        mVoice.setPitch(mPitchSemitones);
    }
    
    // Filter cutoff
    if (!std::isnan(cutoffLock)) {
        mVoice.setCutoff(cutoffLock);
    } else {
        mVoice.setCutoff(mCutoff);
    }
    
    // Filter resonance
    if (!std::isnan(resonanceLock)) {
        mVoice.setResonance(resonanceLock);
    } else {
        mVoice.setResonance(mResonance);
    }
    
    // Amplitude
    if (!std::isnan(amplitudeLock)) {
        mVoice.setAmplitude(amplitudeLock);
    } else {
        mVoice.setAmplitude(mAmplitude);
    }
    
    // Attack
    if (!std::isnan(attackLock)) {
        mVoice.setAttack(attackLock);
    } else {
        mVoice.setAttack(mAttack);
    }
    
    // Decay
    if (!std::isnan(decayLock)) {
        mVoice.setDecay(decayLock);
    } else {
        mVoice.setDecay(mDecay);
    }
    
    // Sustain
    if (!std::isnan(sustainLock)) {
        mVoice.setSustain(sustainLock);
    } else {
        mVoice.setSustain(mSustain);
    }
    
    // Release
    if (!std::isnan(releaseLock)) {
        mVoice.setRelease(releaseLock);
    } else {
        mVoice.setRelease(mRelease);
    }
}

/**
 * Clear step lock values (return to global parameters)
 */
void SynthPart::clearStepLocks() {
    mVoice.setPitch(mPitchSemitones);
    mVoice.setCutoff(mCutoff);
    mVoice.setResonance(mResonance);
    mVoice.setAmplitude(mAmplitude);
    mVoice.setAttack(mAttack);
    mVoice.setDecay(mDecay);
    mVoice.setSustain(mSustain);
    mVoice.setRelease(mRelease);
}

} // namespace Tribex