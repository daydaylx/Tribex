#include "WavetableSynth.h"
#include <cstring>
#include <cmath>

namespace Tribex {

// Static members
float WavetableSynthVoice::sSampleRate = 44100.0f;

// Precomputed wavetable storage (static to share between all voices)
static float sWavetables[NUM_WAVETABLES][WAVETABLE_SIZE];
static bool sWavetablesInitialized = false;

/**
 * Precompute all wavetables
 * Call once at initialization
 */
void WavetableSynthVoice::precomputeWavetables() {
    if (sWavetablesInitialized) {
        return;
    }
    
    // Constants
    const float TWO_PI = 6.28318530718f;
    const float INVERSE_SIZE = 1.0f / WAVETABLE_SIZE;
    
    // Precompute each wavetable
    for (uint32_t type = 0; type < NUM_WAVETABLES; type++) {
        for (uint32_t i = 0; i < WAVETABLE_SIZE; i++) {
            float phase = i * TWO_PI * INVERSE_SIZE;
            float sample = 0.0f;
            
            switch (static_cast<WavetableType>(type)) {
                case WavetableType::SAW: {
                    // Sawtooth wave
                    sample = -1.0f + (2.0f * i / WAVETABLE_SIZE);
                    break;
                }
                case WavetableType::SQUARE: {
                    // Square wave (50% duty cycle)
                    sample = (i < WAVETABLE_SIZE / 2) ? 0.7f : -0.7f;
                    break;
                }
                case WavetableType::SINE: {
                    // Sine wave
                    sample = std::sin(phase);
                    break;
                }
                case WavetableType::CHORD_MAJOR: {
                    // Major triad: Root + Major 3rd + Perfect 5th
                    // Ratios: 1.0, 1.25, 1.5
                    sample = (std::sin(phase) + 
                             std::sin(phase * 1.25f) + 
                             std::sin(phase * 1.5f)) * 0.33f;
                    break;
                }
                case WavetableType::CHORD_MINOR: {
                    // Minor triad: Root + Minor 3rd + Perfect 5th
                    // Ratios: 1.0, 1.2, 1.5
                    sample = (std::sin(phase) + 
                             std::sin(phase * 1.2f) + 
                             std::sin(phase * 1.5f)) * 0.33f;
                    break;
                }
                case WavetableType::CHORD_7TH: {
                    // Dominant 7th: Root + Major 3rd + Perfect 5th + Minor 7th
                    // Ratios: 1.0, 1.25, 1.5, 1.75
                    sample = (std::sin(phase) + 
                             std::sin(phase * 1.25f) + 
                             std::sin(phase * 1.5f) + 
                             std::sin(phase * 1.75f)) * 0.25f;
                    break;
                }
            }
            
            sWavetables[type][i] = sample;
        }
    }
    
    sWavetablesInitialized = true;
}

/**
 * Get precomputed wavetable pointer
 */
const float* WavetableSynthVoice::getWavetable(WavetableType type) {
    if (!sWavetablesInitialized) {
        precomputeWavetables();
    }
    uint32_t index = static_cast<uint32_t>(type);
    if (index >= NUM_WAVETABLES) {
        index = 0;  // Default to saw
    }
    return sWavetables[index];
}

WavetableSynthVoice::WavetableSynthVoice()
    : mPlaying(false)
    , mWavetable(WavetableType::SAW)
    , mPhase(0.0f)
    , mPhaseIncrement(1.0f)
    , mPitchSemitones(0.0f)
    , mLow(0.0f)
    , mHigh(0.0f)
    , mBand(0.0f)
    , mCutoff(0.8f)
    , mResonance(0.0f)
    , mADSRState(ADSRState::IDLE)
    , mEnvelope(0.0f)
    , mAttack(0.01f)      // 10ms
    , mDecay(0.2f)       // 200ms
    , mSustain(0.7f)      // 70%
    , mRelease(0.3f)      // 300ms
    , mAmplitude(1.0f)
{
    // Initialize wavetables if not done
    if (!sWavetablesInitialized) {
        precomputeWavetables();
    }
}

void WavetableSynthVoice::start(float amplitude) {
    mPlaying = true;
    mADSRState = ADSRState::ATTACK;
    mEnvelope = 0.0f;
    mAmplitude = amplitude;
    mPhase = 0.0f;  // Reset phase
    
    // Reset filter
    mLow = 0.0f;
    mHigh = 0.0f;
    mBand = 0.0f;
}

void WavetableSynthVoice::stop() {
    if (mADSRState != ADSRState::IDLE) {
        mADSRState = ADSRState::RELEASE;
    }
}

void WavetableSynthVoice::renderSample(float& left, float& right) {
    if (!mPlaying) {
        left = 0.0f;
        right = 0.0f;
        return;
    }
    
    // Update ADSR envelope
    const float dt = 1.0f / sSampleRate;
    mEnvelope = updateADSR(dt);
    
    // Check if voice finished
    if (mADSRState == ADSRState::IDLE) {
        mPlaying = false;
        left = 0.0f;
        right = 0.0f;
        return;
    }
    
    // Read from wavetable
    float sample = readWavetable(mPhase);
    
    // Apply filter
    sample = processFilter(sample);
    
    // Apply envelope and amplitude
    sample *= mEnvelope * mAmplitude;
    
    // Update phase
    mPhase += mPhaseIncrement;
    if (mPhase >= WAVETABLE_SIZE) {
        mPhase -= WAVETABLE_SIZE;
    }
    
    // Output (mono -> stereo)
    left = sample;
    right = sample;
}

float WavetableSynthVoice::readWavetable(float phase) {
    // Linear interpolation
    uint32_t index1 = static_cast<uint32_t>(phase);
    uint32_t index2 = (index1 + 1) % WAVETABLE_SIZE;
    float frac = phase - index1;
    
    const float* table = sWavetables[static_cast<uint32_t>(mWavetable)];
    
    // Linear interpolation: sample = y1 + frac * (y2 - y1)
    float sample1 = table[index1];
    float sample2 = table[index2];
    
    return sample1 + frac * (sample2 - sample1);
}

float WavetableSynthVoice::updateADSR(float dt) {
    const float attackRate = 1.0f / mAttack;
    const float decayRate = (1.0f - mSustain) / mDecay;
    const float releaseRate = 1.0f / mRelease;
    
    switch (mADSRState) {
        case ADSRState::ATTACK:
            mEnvelope += attackRate * dt;
            if (mEnvelope >= 1.0f) {
                mEnvelope = 1.0f;
                mADSRState = ADSRState::DECAY;
            }
            break;
            
        case ADSRState::DECAY:
            mEnvelope -= decayRate * dt;
            if (mEnvelope <= mSustain) {
                mEnvelope = mSustain;
                mADSRState = ADSRState::SUSTAIN;
            }
            break;
            
        case ADSRState::SUSTAIN:
            // Sustain level is constant
            break;
            
        case ADSRState::RELEASE:
            mEnvelope -= releaseRate * dt;
            if (mEnvelope <= 0.0f) {
                mEnvelope = 0.0f;
                mADSRState = ADSRState::IDLE;
            }
            break;
            
        case ADSRState::IDLE:
            mEnvelope = 0.0f;
            break;
    }
    
    return mEnvelope;
}

/**
 * State Variable Filter (Resonant Lowpass)
 * 
 * TPT (Topology Preserving) form for audio stability.
 * Based on Andrew Simper's SVF design.
 */
float WavetableSynthVoice::processFilter(float input) {
    // Convert normalized cutoff to frequency (Hz)
    const float minCutoff = 40.0f;     // 40 Hz
    const float maxCutoff = 18000.0f;  // 18 kHz
    float cutoffHz = minCutoff + mCutoff * (maxCutoff - minCutoff);
    
    // Calculate filter coefficient (TPT form)
    // g = tan(pi * cutoff / sample_rate)
    float g = std::tan(3.14159265359f * cutoffHz / sSampleRate);
    
    // Resonance damping (1 to 1e-6)
    float resonance = mResonance;  // 0-1
    float r = 1.0f - resonance * 0.999f;  // 1.0 to 0.001
    
    // TPT integrator coefficients
    float gDivR = g / r;
    float gDivR2 = g * gDivR;
    
    // TPT SVF equations
    // Highpass
    float high = (input - mLow - r * mBand);
    
    // Bandpass
    float band = gDivR * high + mBand;
    
    // Lowpass
    float low = gDivR2 * high + mBand + mLow;
    
    // Update state
    mLow = low;
    mHigh = high;
    mBand = band;
    
    // Output lowpass
    return low;
}

void WavetableSynthVoice::setWavetable(WavetableType type) {
    mWavetable = type;
}

void WavetableSynthVoice::setPitch(float semitones) {
    mPitchSemitones = semitones;
    
    // Calculate phase increment
    // base frequency = 440 Hz (A4)
    // pitch adjustment = 2^(semitones/12)
    float baseFreq = 440.0f * std::pow(2.0f, semitones / 12.0f);
    mPhaseIncrement = baseFreq * WAVETABLE_SIZE / sSampleRate;
}

void WavetableSynthVoice::setCutoff(float normalizedCutoff) {
    mCutoff = normalizedCutoff;
}

void WavetableSynthVoice::setResonance(float resonance) {
    mResonance = resonance;
}

void WavetableSynthVoice::setAmplitude(float amplitude) {
    mAmplitude = amplitude;
}

void WavetableSynthVoice::setAttack(float attackMs) {
    mAttack = attackMs / 1000.0f;  // Convert to seconds
}

void WavetableSynthVoice::setDecay(float decayMs) {
    mDecay = decayMs / 1000.0f;
}

void WavetableSynthVoice::setSustain(float sustainLevel) {
    mSustain = sustainLevel;
}

void WavetableSynthVoice::setRelease(float releaseMs) {
    mRelease = releaseMs / 1000.0f;
}

} // namespace Tribex