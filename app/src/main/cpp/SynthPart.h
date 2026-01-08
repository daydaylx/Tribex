#ifndef TRIBEX_SYNTHPART_H
#define TRIBEX_SYNTHPART_H

#include "WavetableSynth.h"
#include <cstdint>
#include <atomic>

namespace Tribex {

/**
 * Synth Part - Wavetable Monosynth (Part 8)
 * 
 * This is called from audio thread - NO ALLOCATIONS!
 * 
 * Features:
 * - Monophonic (1 voice)
 * - 6 Wavetables (saw, square, sine, chord-major, chord-minor, chord-7th)
 * - ADSR Envelope (Attack, Decay, Sustain, Release)
 * - State Variable Filter (Resonant LP)
 * - Pitch adjustment (-24 to +24 semitones)
 */

/**
 * Synth Part
 * 
 * Manages:
 * - One monophonic voice (no voice stealing needed)
 * - Per-part parameters (wavetable, pitch, filter, ADSR)
 * - Trigger handling from sequencer
 */
class SynthPart {
public:
    SynthPart();
    ~SynthPart();
    
    /**
     * Initialize synth (call once at audio engine startup)
     * @param sampleRate Audio sample rate
     */
    void initialize(float sampleRate);
    
    /**
     * Trigger voice (called from sequencer)
     * @param velocity Note velocity (0.0 to 1.0)
     */
    void trigger(float velocity);
    
    /**
     * Render voice (called from audio thread)
     * CRITICAL: NO ALLOCATIONS!
     * @param leftBuffer Left output buffer
     * @param rightBuffer Right output buffer
     * @param numFrames Number of frames to render
     */
    void render(float* leftBuffer, float* rightBuffer, int32_t numFrames);
    
    /**
     * Stop voice (release phase)
     */
    void stop();
    
    // Parameter updates (non-blocking, atomic)
    void setWavetable(WavetableType type);
    void setPitch(float semitones);
    void setCutoff(float normalizedCutoff);
    void setResonance(float resonance);
    void setAmplitude(float amplitude);
    void setAttack(float attackMs);
    void setDecay(float decayMs);
    void setSustain(float sustainLevel);
    void setRelease(float releaseMs);
    
    // State queries
    bool isPlaying() const { return mVoice.isPlaying(); }
    
    // Mute/Solo (M5: UI only, no audio effect yet)
    void setMute(bool muted) { mMuted.store(muted, std::memory_order_relaxed); }
    void setSolo(bool solo) { mSoloed.store(solo, std::memory_order_relaxed); }
    bool isMuted() const { return mMuted.load(); }
    bool isSoloed() const { return mSoloed.load(); }
    
    /**
     * Apply step lock values
     * @param pitchLock Pitch in semitones (NaN = no lock)
     * @param cutoffLock Filter cutoff (NaN = no lock)
     * @param resonanceLock Filter resonance (NaN = no lock)
     * @param amplitudeLock Amplitude (NaN = no lock)
     * @param attackLock Attack time in ms (NaN = no lock)
     * @param decayLock Decay time in ms (NaN = no lock)
     * @param sustainLock Sustain level (NaN = no lock)
     * @param releaseLock Release time in ms (NaN = no lock)
     */
    void applyStepLocks(float pitchLock, float cutoffLock, float resonanceLock,
                      float amplitudeLock, float attackLock, float decayLock,
                      float sustainLock, float releaseLock);
    
    /**
     * Clear step lock values (return to global parameters)
     */
    void clearStepLocks();
    
private:
    // Monophonic voice (no voice stealing needed)
    WavetableSynthVoice mVoice;
    
    // Global parameters (step defaults)
    WavetableType mWavetable;
    float mPitchSemitones;
    float mCutoff;
    float mResonance;
    float mAmplitude;
    float mAttack;
    float mDecay;
    float mSustain;
    float mRelease;
    
    // Mute/Solo state (M5: UI only)
    std::atomic<bool> mMuted;
    std::atomic<bool> mSoloed;
    
    // Apply a parameter value if lock is valid (not NaN)
    inline void applyLock(float lockValue, float& globalValue, void (WavetableSynthVoice::*setter)(float));
    inline void applyLock(float lockValue, float& globalValue, void (WavetableSynthVoice::*setter)(float), float scale = 1.0f);
};

} // namespace Tribex

#endif // TRIBEX_SYNTHPART_H