#pragma once

#include <memory>
#include <atomic>
#include <array>
#include <string>

#include "AudioEvents.h"
#include "EventQueue.h"
#include "SamplePart.h"
#include "SynthPart.h"
#include "Sequencer.h"
#include "FXManager.h"
#include "oboe/Oboe.h"

// QA: Telemetry constants
constexpr int32_t MAX_I16_BUFFER_SIZE = 2048;  // Max frames * channels

class AudioEngine : public oboe::AudioStreamDataCallback,
                    public oboe::AudioStreamErrorCallback {
public:
    AudioEngine();
    ~AudioEngine();

    // Audio lifecycle
    bool start();
    bool stop();
    bool isPlaying() const;

    // Master control
    void setMasterGain(float gain);
    void setMasterPan(float pan);
    void setTestToneFrequency(float freq);

    // Sequencer control
    void setBPM(float bpm);
    void clearEvents();
    void startSequencer();
    void stopSequencer();
    bool isSequencerPlaying() const;
    
    // Pattern management (direct call, not via event queue - patterns are too large)
    void setPattern(const Tribex::Pattern& pattern);
    
    // P0.4: Sequencer state getters (thread-safe via atomics)
    uint32_t getCurrentStep() const;
    uint32_t getLoopIteration() const;

    // M4: Sample Engine methods
    void loadSample(uint32_t partIndex, const Tribex::SampleData& sample);
    void unloadSample(uint32_t partIndex);
    void setVoicePitch(uint32_t partIndex, float pitch);
    void setVoicePan(uint32_t partIndex, float pan);
    void setVoiceLevel(uint32_t partIndex, float level);
    void setVoiceDecay(uint32_t partIndex, float decayMs);
    void setVoiceFilter(uint32_t partIndex, Tribex::FilterType filter);

    // M5: Part control
    void setPartMute(uint32_t partIndex, bool muted);
    void setPartSolo(uint32_t partIndex, bool solo);

    // M5: Synth Part control methods (Part 8 only)
    void setSynthWavetable(uint32_t partIndex, uint8_t type);
    void setSynthCutoff(uint32_t partIndex, float cutoff);
    void setSynthResonance(uint32_t partIndex, float resonance);
    void setSynthAttack(uint32_t partIndex, float attackMs);
    void setSynthDecay(uint32_t partIndex, float decayMs);
    void setSynthSustain(uint32_t partIndex, float sustainLevel);
    void setSynthRelease(uint32_t partIndex, float releaseMs);

    // M6: FX Control methods
    void setDelayTimeMs(float timeMs);
    void setDelayFeedback(float feedback);
    void setDelayMix(float mix);
    void setReverbSize(float size);
    void setReverbDensity(float density);
    void setReverbMix(float mix);
    void setValveAmount(float amount);
    void setLimiterThresholdDb(float thresholdDb);
    void setLimiterReleaseMs(float releaseMs);

    // M6: Degradation control
    void setDegradationLevel(int level);
    int getDegradationLevel() const;
    int32_t getMaxVoices() const;
    void resetXRunCounter();

    // QA: Error telemetry getters (thread-safe)
    uint32_t getInvalidEventCount() const;
    uint32_t getLastInvalidEventType() const;
    void resetInvalidEventCount();

    // Audio callback (Oboe)
    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream *stream,
        void *audioData,
        int32_t numFrames) override;

    // Error callback
    void onErrorAfterClose(oboe::AudioStream *stream, oboe::Result result) override;

private:
    // Oboe stream management
    oboe::Result openStream();
    oboe::Result closeStream();

    // Test tone generation
    void generateSine(float *outputBuffer, int32_t numFrames, int32_t numChannels);

    // Event queue processing
    void processEvents();

    // State
    std::atomic<bool> mIsPlaying;
    std::atomic<bool> mShouldRestart;
    oboe::ManagedStream mStream;
    LockFreeQueue<AudioEvent, 32> mEventQueue;
    std::atomic<float> mMasterGain;
    std::atomic<float> mMasterPan;
    std::atomic<float> mTestToneFrequency;
    std::atomic<double> mPhase;
    Tribex::Sequencer mSequencer;
    std::atomic<int64_t> mSampleCounter;
    std::atomic<bool> mAnyPartSoloed;

    // QA: Error telemetry counters (atomics, lock-free)
    std::atomic<uint32_t> mInvalidEventCount;
    std::atomic<uint32_t> mLastInvalidEventType;

    // M4.5: Preallocated buffers for drum parts (no VLAs!)
    static constexpr int32_t MAX_FRAMES = 1024;
    std::array<float, MAX_FRAMES> mPartLeftBuffer;
    std::array<float, MAX_FRAMES> mPartRightBuffer;

    // M5: Preallocated buffers for synth part
    std::array<float, MAX_FRAMES> mSynthLeftBuffer;
    std::array<float, MAX_FRAMES> mSynthRightBuffer;

    // P0.2: Preallocated buffers for FX chain (no stack arrays)
    std::array<float, MAX_FRAMES> mFXLeftBuffer;
    std::array<float, MAX_FRAMES> mFXRightBuffer;

    // QA: Preallocated I16 temp buffer for VLA fix
    std::array<float, MAX_I16_BUFFER_SIZE> mI16TempBuffer;

    // Voice parts
    static constexpr uint32_t NUM_DRUM_PARTS = 8;
    Tribex::SamplePart mSampleParts[NUM_DRUM_PARTS];

    // M5: Synth part (Part 8)
    Tribex::SynthPart mSynthPart;

    // M6: FX Manager
    Tribex::FXManager mFXManager;

    // M2 NEU: Constants
    static constexpr int32_t MAX_TRIGGERS_PER_CALLBACK = 16;
};
