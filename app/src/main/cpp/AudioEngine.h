#ifndef TRIBEX_AUDIOENGINE_H
#define TRIBEX_AUDIOENGINE_H

#include <atomic>
#include <oboe/Oboe.h>
#include <cstdint>
#include <memory>
#include "EventQueue.h"
#include "AudioEvents.h"
#include "PatternData.h"
#include "Sequencer.h"
#include "SamplePart.h"
#include "SynthPart.h"
#include "SampleLoader.h"

class AudioEngine : public oboe::AudioStreamDataCallback,
                    public oboe::AudioStreamErrorCallback {
public:
    AudioEngine();
    ~AudioEngine();

    // Lifecycle - called from UI thread via JNI
    bool start();
    bool stop();
    bool isPlaying() const;

    // Event control - called from UI thread via JNI (non-blocking)
    void setMasterGain(float gain);
    void setMasterPan(float pan);
    void setTestToneFrequency(float freq);
    void setBPM(float bpm);  // M2 NEU
    void clearEvents();
    
    // Sequencer control - M2 NEU
    void startSequencer();
    void stopSequencer();
    bool isSequencerPlaying() const;

    // M4: Sample Engine control (called from UI thread via JNI, non-blocking)
    void loadSample(uint32_t partIndex, const Tribex::SampleData& sample);
    void unloadSample(uint32_t partIndex);
    void setVoicePitch(uint32_t partIndex, float pitch);
    void setVoicePan(uint32_t partIndex, float pan);
    void setVoiceLevel(uint32_t partIndex, float level);
    void setVoiceDecay(uint32_t partIndex, float decayMs);
    void setVoiceFilter(uint32_t partIndex, Tribex::FilterType filter);
    void setPartMute(uint32_t partIndex, bool muted);
    void setPartSolo(uint32_t partIndex, bool solo);
    
    // M5: Synth Part control (Part 8 only)
    void setSynthWavetable(uint32_t partIndex, uint8_t type);  // 0-5: saw, square, sine, maj, min, 7th
    void setSynthCutoff(uint32_t partIndex, float cutoff);     // Normalized 0-1
    void setSynthResonance(uint32_t partIndex, float resonance); // 0-1
    void setSynthAttack(uint32_t partIndex, float attackMs);
    void setSynthDecay(uint32_t partIndex, float decayMs);
    void setSynthSustain(uint32_t partIndex, float sustainLevel);
    void setSynthRelease(uint32_t partIndex, float releaseMs);

    // Oboe audio callback (called from audio thread)
    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream *stream,
        void *audioData,
        int32_t numFrames
    );

    // Error callback
    void onErrorAfterClose(oboe::AudioStream *stream, oboe::Result result);

private:
    // Stream management
    oboe::Result openStream();
    oboe::Result closeStream();

    // Audio generation (realtime, no allocations!)
    void generateSine(float *outputBuffer, int32_t numFrames, int32_t numChannels);
    
    // Event processing (realtime, no allocations!)
    void processEvents();

    // State
    std::atomic<bool> mIsPlaying;
    std::atomic<bool> mShouldRestart;
    
    // Stream
    oboe::ManagedStream mStream;

    // Event queue (lock-free SPSC)
    LockFreeQueue<AudioEvent, 256> mEventQueue;

    // Audio parameters (atomic for lock-free access)
    std::atomic<float> mMasterGain;
    std::atomic<float> mMasterPan;
    std::atomic<float> mTestToneFrequency;

    // Sine wave generation (phase accumulation)
    std::atomic<double> mPhase;
    static constexpr double kTwoPi = 6.283185307179586;
    
    // M2: Sequencer
    Tribex::Sequencer mSequencer;
    std::atomic<int64_t> mSampleCounter;  // Global sample counter for timing
    static constexpr int32_t MAX_TRIGGERS_PER_CALLBACK = Tribex::NUM_PARTS;
    
    // M4: Sample Parts (8 drum parts)
    Tribex::SamplePart mSampleParts[8];  // Parts 0-7: Drum parts
    
    // M5: Synth Part (Part 8)
    Tribex::SynthPart mSynthPart;  // Part 8: Synth part
};

#endif // TRIBEX_AUDIOENGINE_H