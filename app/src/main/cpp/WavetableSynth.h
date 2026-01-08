#ifndef TRIBEX_WAVETABLESYNTH_H
#define TRIBEX_WAVETABLESYNTH_H

#include <cstdint>
#include <cmath>

namespace Tribex {

/**
 * Wavetable Synth Part (M5)
 * 
 * Monosynth with pre-calculated wavetables.
 * Chord tables cost same CPU as mono voice (single table lookup).
 * 
 * Features:
 * - 6 Wavetables: Saw, Square, Sine, Chord-Major, Chord-Minor, Chord-7th
 * - Linear interpolation table lookup
 * - ADSR Envelope
 * - State Variable Filter (Resonant LP)
 * - Pitch (-24 to +24 semitones)
 */

// Wavetable Configuration
constexpr uint32_t WAVETABLE_SIZE = 2048;  // Samples per table
constexpr uint32_t NUM_WAVETABLES = 6;     // 6 total tables

// Wavetable Types
enum class WavetableType : uint8_t {
    SAW = 0,
    SQUARE = 1,
    SINE = 2,
    CHORD_MAJOR = 3,
    CHORD_MINOR = 4,
    CHORD_7TH = 5
};

/**
 * Wavetable Synth Voice
 * 
 * Monophonic voice for synth part (Part 8).
 * No voice stealing needed (only one voice).
 */
class WavetableSynthVoice {
public:
    WavetableSynthVoice();
    
    /**
     * Start voice
     * @param amplitude Initial amplitude (0-1)
     */
    void start(float amplitude);
    
    /**
     * Stop voice (release phase)
     */
    void stop();
    
    /**
     * Render one sample
     * @param left Left output sample
     * @param right Right output sample
     */
    void renderSample(float& left, float& right);
    
    /**
     * Is voice playing (including release)
     */
    bool isPlaying() const { return mPlaying; }
    
    // Parameter updates (non-blocking, atomic)
    void setWavetable(WavetableType type);
    void setPitch(float semitones);
    void setCutoff(float normalizedCutoff);  // 0-1
    void setResonance(float resonance);      // 0-1
    void setAmplitude(float amplitude);
    
    // ADSR Parameters
    void setAttack(float attackMs);
    void setDecay(float decayMs);
    void setSustain(float sustainLevel);
    void setRelease(float releaseMs);
    
    // Get precomputed wavetable pointer
    static const float* getWavetable(WavetableType type);

private:
    /**
     * Read from wavetable with linear interpolation
     * @param phase Phase (0.0 to WAVETABLE_SIZE)
     */
    float readWavetable(float phase);
    
    /**
     * Update ADSR envelope
     * @param dt Delta time in seconds
     * @return Current envelope level (0-1)
     */
    float updateADSR(float dt);
    
    /**
     * Process filter
     * @param input Input sample
     * @return Filtered output
     */
    float processFilter(float input);
    
    // Voice state
    bool mPlaying;
    
    // Oscillator
    WavetableType mWavetable;
    float mPhase;          // Current phase (0.0 to WAVETABLE_SIZE)
    float mPhaseIncrement; // Phase increment per sample
    float mPitchSemitones;
    
    // Filter (State Variable Filter - Resonant LP)
    float mLow;           // Lowpass output
    float mHigh;          // Highpass output
    float mBand;          // Bandpass output
    float mCutoff;        // Normalized cutoff (0-1)
    float mResonance;     // Resonance (0-1)
    
    // ADSR Envelope
    enum class ADSRState : uint8_t {
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE,
        IDLE
    };
    
    ADSRState mADSRState;
    float mEnvelope;       // Current envelope level (0-1)
    float mAttack;         // Attack time in seconds
    float mDecay;          // Decay time in seconds
    float mSustain;        // Sustain level (0-1)
    float mRelease;        // Release time in seconds
    float mAmplitude;     // Amplitude (0-1)
    
    // Audio engine sample rate (set once at initialization)
    static float sSampleRate;
    
    /**
     * Set sample rate (call once at initialization)
     */
    static void setSampleRate(float sr) { sSampleRate = sr; }
    
    /**
     * Precompute all wavetables (call once at initialization)
     */
    static void precomputeWavetables();
    
    // Allow SynthPart to access setSampleRate and precomputeWavetables
    friend class SynthPart;
};

} // namespace Tribex

#endif // TRIBEX_WAVETABLESYNTH_H