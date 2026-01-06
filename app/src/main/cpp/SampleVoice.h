#ifndef TRIBEX_SAMPLEVOICE_H
#define TRIBEX_SAMPLEVOICE_H

#include <atomic>
#include <cstdint>
#include <cmath>

namespace Tribex {

/**
 * Sample Voice - Single Voice with Linear Interpolation Resampling
 * 
 * This is called from audio thread - NO ALLOCATIONS!
 * 
 * Voice State:
 * - Playing/Idle
 * - Current sample position (float for fractional positioning)
 * - Playback parameters (pitch, pan, level, decay)
 * - Filter state
 */

// Voice State
enum class VoiceState : uint8_t {
    IDLE = 0,
    PLAYING,
    RELEASING  // In decay phase
};

// Filter Type (DJ-style 1-knob)
enum class FilterType : uint8_t {
    LP = 0,   // Low Pass
    HP = 1    // High Pass
};

/**
 * Sample Voice
 * 
 * Handles playback of a single sample with:
 * - Linear interpolation resampling
 * - Stereo panning (constant power)
 * - Decay envelope (AHD)
 * - DJ-style filter (LP/HP)
 */
class SampleVoice {
public:
    SampleVoice();
    ~SampleVoice();
    
    // Lifecycle
    void start(const float* sampleData, uint32_t length, uint32_t sampleRate,
              float velocity);
    void stop();
    bool isPlaying() const { return mState.load() != VoiceState::IDLE; }
    
    // Render (called from audio thread)
    // Processes one sample and returns output (stereo pair)
    // CRITICAL: NO ALLOCATIONS!
    void renderSample(float& left, float& right);
    
    // Parameter updates (non-blocking, atomic)
    void setPitch(float pitch) { mPitch.store(pitch, std::memory_order_relaxed); }
    void setPan(float pan) { mPan.store(pan, std::memory_order_relaxed); }
    void setLevel(float level) { mLevel.store(level, std::memory_order_relaxed); }
    void setDecay(float decayMs) { mDecayMs.store(decayMs, std::memory_order_relaxed); }
    void setFilter(FilterType filter) { mFilterType.store(filter, std::memory_order_relaxed); }
    
    // State query
    VoiceState getState() const { return mState.load(); }
    uint32_t getPartIndex() const { return mPartIndex.load(); }

private:
    // Calculate pitch ratio from semitones
    float calculatePitchRatio(float semitones) const;
    
    // Apply decay envelope
    float applyDecay(float sample, float position, float totalLength);
    
    // Apply DJ-style filter
    float applyFilter(float sample);
    
    // Advance sample position with pitch
    void advancePosition(float pitchRatio);
    
    // Atomic state
    std::atomic<VoiceState> mState;
    std::atomic<uint32_t> mPartIndex;
    
    // Sample data (pointer - not owned by voice)
    const float* mSampleData;
    uint32_t mSampleLength;
    uint32_t mSampleRate;
    
    // Playback position (float for fractional resampling)
    float mPosition;
    
    // Playback parameters (atomic for lock-free updates)
    std::atomic<float> mPitch;
    std::atomic<float> mPan;
    std::atomic<float> mLevel;
    std::atomic<float> mDecayMs;
    std::atomic<FilterType> mFilterType;
    
    // Decay state
    float mDecayEnvelope;
    float mDecayPhase;
    
    // Filter state (biquad for DJ-style LP/HP)
    float mFilterX1;
    float mFilterX2;
    float mFilterY1;
    float mFilterY2;
    
    // Trigger velocity (for level scaling)
    float mVelocity;
};

} // namespace Tribex

#endif // TRIBEX_SAMPLEVOICE_H