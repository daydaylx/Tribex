#include "AudioEngine.h"
#include <android/log.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <ctime>

#define TAG "AudioEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

constexpr double kTwoPi = 6.283185307179586476925286766559;

AudioEngine::AudioEngine()
    : mIsPlaying(false)
    , mShouldRestart(false)
    , mEventQueue()
    , mMasterGain(1.0f)
    , mMasterPan(0.0f)
    , mTestToneFrequency(440.0f)
    , mPhase(0.0)
    , mSequencer()
    , mSampleCounter(0)
    , mAnyPartSoloed(false)
    , mSampleParts{0, 1, 2, 3, 4, 5, 6, 7}  // Initialize 8 drum parts
{
    // M4.5: Zero all preallocated buffers
    mPartLeftBuffer.fill(0.0f);
    mPartRightBuffer.fill(0.0f);
    mSynthLeftBuffer.fill(0.0f);
    mSynthRightBuffer.fill(0.0f);
    // P0.2: Zero FX chain buffers
    mFXLeftBuffer.fill(0.0f);
    mFXRightBuffer.fill(0.0f);
}

AudioEngine::~AudioEngine() {
    stop();
}

bool AudioEngine::start() {
    if (mIsPlaying.load()) {
        LOGI("Audio already playing");
        return true;
    }

    LOGI("Starting audio engine");
    
    auto result = openStream();
    if (result != oboe::Result::OK) {
        LOGE("Failed to open stream: %s", oboe::convertToText(result));
        return false;
    }

    // M5: Initialize synth part
    if (mStream) {
        float sampleRate = mStream->getSampleRate();
        mSynthPart.initialize(sampleRate);
        mFXManager.setSampleRate(sampleRate);
    }

    result = mStream->requestStart();
    if (result != oboe::Result::OK) {
        LOGE("Failed to start stream: %s", oboe::convertToText(result));
        closeStream();
        return false;
    }

    mIsPlaying.store(true);
    // Reset sample counter on start
    mSampleCounter.store(0, std::memory_order_relaxed);
    LOGI("Audio started successfully");
    return true;
}

bool AudioEngine::stop() {
    if (!mIsPlaying.load()) {
        LOGI("Audio already stopped");
        return true;
    }

    LOGI("Stopping audio engine");

    if (mStream) {
        mStream->stop();
        mStream->close();
    }

    mIsPlaying.store(false);
    mPhase.store(0.0); // Reset phase on stop
    mSequencer.stop(); // Stop sequencer
    for (uint32_t i = 0; i < 8; i++) {
        mSampleParts[i].releaseRetiredSamples();
    }
    LOGI("Audio stopped");
    return true;
}

bool AudioEngine::isPlaying() const {
    return mIsPlaying.load();
}

void AudioEngine::setMasterGain(float gain) {
    // Clamp gain to reasonable range (0.0 to 2.0)
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 2.0f) gain = 2.0f;
    
    AudioEvent event(EventType::SET_MASTER_GAIN, gain);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - gain event dropped");
    }
}

void AudioEngine::setMasterPan(float pan) {
    // Clamp pan to valid range (-1.0 to 1.0)
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f) pan = 1.0f;
    
    AudioEvent event(EventType::SET_MASTER_PAN, pan);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - pan event dropped");
    }
}

void AudioEngine::setTestToneFrequency(float freq) {
    // Clamp frequency to reasonable range (20Hz to 20kHz)
    if (freq < 20.0f) freq = 20.0f;
    if (freq > 20000.0f) freq = 20000.0f;
    
    AudioEvent event(EventType::SET_TEST_TONE_FREQ, freq);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - frequency event dropped");
    }
}

void AudioEngine::setBPM(float bpm) {
    // M2 NEU: Set BPM for sequencer
    // Clamp to valid range (20.0 to 300.0)
    if (bpm < 20.0f) bpm = 20.0f;
    if (bpm > 300.0f) bpm = 300.0f;
    
    AudioEvent event(EventType::SET_BPM, bpm);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - BPM event dropped");
    }
}

void AudioEngine::clearEvents() {
    mEventQueue.clear();
}

void AudioEngine::startSequencer() {
    mSequencer.start();
}

void AudioEngine::stopSequencer() {
    mSequencer.stop();
}

bool AudioEngine::isSequencerPlaying() const {
    return mSequencer.isPlaying();
}

// Pattern management
void AudioEngine::setPattern(const Tribex::Pattern& pattern) {
    // Direct call - patterns are too large for event queue
    // This is called from control thread, but sequencer.loadPattern() is thread-safe
    mSequencer.loadPattern(pattern);
}

// P0.4: Sequencer state getters
uint32_t AudioEngine::getCurrentStep() const {
    return mSequencer.getCurrentStep();
}

uint32_t AudioEngine::getLoopIteration() const {
    return mSequencer.getLoopIteration();
}

// M4: Sample Engine methods
void AudioEngine::loadSample(uint32_t partIndex, const Tribex::SampleData& sample) {
    if (partIndex >= 8) {  // Only drum parts (0-7) load samples
        LOGE("Invalid part index for sample loading: %d", partIndex);
        return;
    }

    // Load sample into part (called from IO thread, allocations allowed)
    bool deferFree = mIsPlaying.load(std::memory_order_relaxed);
    mSampleParts[partIndex].loadSample(sample, deferFree);
    
    LOGI("Loaded sample %d into part %d", sample.id, partIndex);
}

void AudioEngine::unloadSample(uint32_t partIndex) {
    if (partIndex >= 8) {  // Only drum parts (0-7) have samples
        LOGE("Invalid part index for sample unloading: %d", partIndex);
        return;
    }

    bool deferFree = mIsPlaying.load(std::memory_order_relaxed);
    mSampleParts[partIndex].unloadSample(deferFree);
    
    LOGI("Unloaded sample from part %d", partIndex);
}

void AudioEngine::setVoicePitch(uint32_t partIndex, float pitch) {
    if (partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    // Clamp pitch to valid range (-24 to +24 semitones)
    if (pitch < -24.0f) pitch = -24.0f;
    if (pitch > 24.0f) pitch = 24.0f;
    
    AudioEvent event(EventType::SET_VOICE_PITCH, pitch, partIndex);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - pitch event dropped");
    }
}

void AudioEngine::setVoicePan(uint32_t partIndex, float pan) {
    if (partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    // Clamp pan to valid range (-1.0 to 1.0)
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f) pan = 1.0f;
    
    AudioEvent event(EventType::SET_VOICE_PAN, pan, partIndex);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - pan event dropped");
    }
}

void AudioEngine::setVoiceLevel(uint32_t partIndex, float level) {
    if (partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    // Clamp level to valid range (0.0 to 1.0)
    if (level < 0.0f) level = 0.0f;
    if (level > 1.0f) level = 1.0f;
    
    AudioEvent event(EventType::SET_VOICE_LEVEL, level, partIndex);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - level event dropped");
    }
}

void AudioEngine::setVoiceDecay(uint32_t partIndex, float decayMs) {
    if (partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    // Clamp decay to valid range (0.0 to 5000.0 ms)
    if (decayMs < 0.0f) decayMs = 0.0f;
    if (decayMs > 5000.0f) decayMs = 5000.0f;
    
    AudioEvent event(EventType::SET_VOICE_DECAY, decayMs, partIndex);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - decay event dropped");
    }
}

void AudioEngine::setVoiceFilter(uint32_t partIndex, Tribex::FilterType filter) {
    if (partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    AudioEvent event(EventType::SET_VOICE_FILTER, static_cast<float>(filter), partIndex);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - filter event dropped");
    }
}

void AudioEngine::setVoiceTrim(uint32_t partIndex, uint32_t startOffset, uint32_t endOffset) {
    if (partIndex >= 8) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    mSampleParts[partIndex].setTrim(startOffset, endOffset);
}

void AudioEngine::setPartMute(uint32_t partIndex, bool muted) {
    if (partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    // M5: Apply to both drum parts and synth part
    if (partIndex < 8) {
        mSampleParts[partIndex].setMute(muted);
    } else if (partIndex == 8) {
        mSynthPart.setMute(muted);
    }
    
    LOGI("Part %d muted: %d", partIndex, muted);
}

void AudioEngine::setPartSolo(uint32_t partIndex, bool solo) {
    if (partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    // M4.5: Apply to both drum parts and synth part
    if (partIndex < 8) {
        mSampleParts[partIndex].setSolo(solo);
    } else if (partIndex == 8) {
        mSynthPart.setSolo(solo);
    }
    
    // M4.5: Check if any part is soloed
    bool anySoloed = false;
    for (uint32_t i = 0; i < 8; i++) {
        if (mSampleParts[i].isSoloed()) {
            anySoloed = true;
            break;
        }
    }
    if (!anySoloed && mSynthPart.isSoloed()) {
        anySoloed = true;
    }
    mAnyPartSoloed.store(anySoloed, std::memory_order_relaxed);
    
    LOGI("Part %d soloed: %d, any part soloed: %d", partIndex, solo, anySoloed);
}

// M5: Synth Part control methods (Part 8 only)
void AudioEngine::setSynthWavetable(uint32_t partIndex, uint8_t type) {
    if (partIndex != 8) {
        LOGE("Synth wavetable only valid for part 8");
        return;
    }
    
    // Clamp to valid range (0-5)
    if (type > 5) type = 5;
    
    Tribex::WavetableType wt = static_cast<Tribex::WavetableType>(type);
    AudioEvent event(EventType::SET_SYNTH_WAVETABLE, static_cast<float>(type), partIndex);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - wavetable event dropped");
    }
}

void AudioEngine::setSynthCutoff(uint32_t partIndex, float cutoff) {
    if (partIndex != 8) {
        LOGE("Synth cutoff only valid for part 8");
        return;
    }
    
    // Clamp to valid range (0.0 to 1.0)
    if (cutoff < 0.0f) cutoff = 0.0f;
    if (cutoff > 1.0f) cutoff = 1.0f;
    
    AudioEvent event(EventType::SET_SYNTH_CUTOFF, cutoff, partIndex);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - cutoff event dropped");
    }
}

void AudioEngine::setSynthResonance(uint32_t partIndex, float resonance) {
    if (partIndex != 8) {
        LOGE("Synth resonance only valid for part 8");
        return;
    }
    
    // Clamp to valid range (0.0 to 1.0)
    if (resonance < 0.0f) resonance = 0.0f;
    if (resonance > 1.0f) resonance = 1.0f;
    
    AudioEvent event(EventType::SET_SYNTH_RESONANCE, resonance, partIndex);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - resonance event dropped");
    }
}

void AudioEngine::setSynthAttack(uint32_t partIndex, float attackMs) {
    if (partIndex != 8) {
        LOGE("Synth attack only valid for part 8");
        return;
    }
    
    // Clamp to valid range (0.0 to 5000.0 ms)
    if (attackMs < 0.0f) attackMs = 0.0f;
    if (attackMs > 5000.0f) attackMs = 5000.0f;
    
    AudioEvent event(EventType::SET_SYNTH_ATTACK, attackMs, partIndex);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - attack event dropped");
    }
}

void AudioEngine::setSynthDecay(uint32_t partIndex, float decayMs) {
    if (partIndex != 8) {
        LOGE("Synth decay only valid for part 8");
        return;
    }
    
    // Clamp to valid range (0.0 to 5000.0 ms)
    if (decayMs < 0.0f) decayMs = 0.0f;
    if (decayMs > 5000.0f) decayMs = 5000.0f;
    
    AudioEvent event(EventType::SET_SYNTH_DECAY, decayMs, partIndex);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - decay event dropped");
    }
}

void AudioEngine::setSynthSustain(uint32_t partIndex, float sustainLevel) {
    if (partIndex != 8) {
        LOGE("Synth sustain only valid for part 8");
        return;
    }
    
    // Clamp to valid range (0.0 to 1.0)
    if (sustainLevel < 0.0f) sustainLevel = 0.0f;
    if (sustainLevel > 1.0f) sustainLevel = 1.0f;
    
    AudioEvent event(EventType::SET_SYNTH_SUSTAIN, sustainLevel, partIndex);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - sustain event dropped");
    }
}

void AudioEngine::setSynthRelease(uint32_t partIndex, float releaseMs) {
    if (partIndex != 8) {
        LOGE("Synth release only valid for part 8");
        return;
    }
    
    // Clamp to valid range (0.0 to 5000.0 ms)
    if (releaseMs < 0.0f) releaseMs = 0.0f;
    if (releaseMs > 5000.0f) releaseMs = 5000.0f;
    
    AudioEvent event(EventType::SET_SYNTH_RELEASE, releaseMs, partIndex);
    if (!mEventQueue.push(event)) {
        LOGE("Event queue full - release event dropped");
    }
}

void AudioEngine::processEvents() {
    // CRITICAL: This is called from audio thread - NO ALLOCATIONS!
    
    AudioEvent event;
    while (mEventQueue.pop(event)) {
        if (!event.isValid()) {
            // P0.3: No logging in audio callback - use error counter instead
            mInvalidEventCount.fetch_add(1, std::memory_order_relaxed);
            mLastInvalidEventType.store(static_cast<uint32_t>(event.type), std::memory_order_relaxed);
            continue;
        }
        
        uint32_t partIndex = event.reserved;
        
        switch (event.type) {
            case EventType::SET_MASTER_GAIN:
                mMasterGain.store(event.value, std::memory_order_relaxed);
                break;
                
            case EventType::SET_MASTER_PAN:
                mMasterPan.store(event.value, std::memory_order_relaxed);
                break;
                
            case EventType::SET_TEST_TONE_FREQ:
                mTestToneFrequency.store(event.value, std::memory_order_relaxed);
                break;
                
            case EventType::SET_BPM:  // M2 NEU
                mSequencer.setBPM(event.value);
                break;
                
            // M4: Sample Engine events
            case EventType::TRIGGER_VOICE:
                if (partIndex < Tribex::NUM_PARTS) {
                    float velocity = Tribex::velocityToFloat(static_cast<uint8_t>(event.value * 3.0f));
                    if (partIndex < 8) {
                        mSampleParts[partIndex].trigger(velocity);
                    } else if (partIndex == 8) {
                        mSynthPart.trigger(velocity);
                    }
                }
                break;
                
            case EventType::SET_VOICE_PITCH:
                if (partIndex < Tribex::NUM_PARTS) {
                    if (partIndex < 8) {
                        mSampleParts[partIndex].setPitch(event.value);
                    } else if (partIndex == 8) {
                        mSynthPart.setPitch(event.value);
                    }
                }
                break;
                
            case EventType::SET_VOICE_PAN:
                if (partIndex < 8) {  // Only drum parts have pan
                    mSampleParts[partIndex].setPan(event.value);
                }
                break;
                
            case EventType::SET_VOICE_LEVEL:
                if (partIndex < Tribex::NUM_PARTS) {
                    if (partIndex < 8) {
                        mSampleParts[partIndex].setLevel(event.value);
                    } else if (partIndex == 8) {
                        mSynthPart.setAmplitude(event.value);
                    }
                }
                break;
                
            case EventType::SET_VOICE_DECAY:
                if (partIndex < Tribex::NUM_PARTS) {
                    if (partIndex < 8) {
                        mSampleParts[partIndex].setDecay(event.value);
                    } else if (partIndex == 8) {
                        mSynthPart.setDecay(event.value);
                    }
                }
                break;
                
            case EventType::SET_VOICE_FILTER:
                if (partIndex < 8) {  // Only drum parts have 1-knob filter
                    Tribex::FilterType filter = (event.value > 0.5f) 
                        ? Tribex::FilterType::HP 
                        : Tribex::FilterType::LP;
                    mSampleParts[partIndex].setFilter(filter);
                }
                break;
                
            // M5: Synth Part events
            case EventType::SET_SYNTH_WAVETABLE:
                if (partIndex == 8) {
                    uint8_t type = static_cast<uint8_t>(event.value);
                    mSynthPart.setWavetable(static_cast<Tribex::WavetableType>(type));
                }
                break;
                
            case EventType::SET_SYNTH_CUTOFF:
                if (partIndex == 8) {
                    mSynthPart.setCutoff(event.value);
                }
                break;
                
            case EventType::SET_SYNTH_RESONANCE:
                if (partIndex == 8) {
                    mSynthPart.setResonance(event.value);
                }
                break;
                
            case EventType::SET_SYNTH_ATTACK:
                if (partIndex == 8) {
                    mSynthPart.setAttack(event.value);
                }
                break;
                
            case EventType::SET_SYNTH_DECAY:
                if (partIndex == 8) {
                    mSynthPart.setDecay(event.value);
                }
                break;
                
            case EventType::SET_SYNTH_SUSTAIN:
                if (partIndex == 8) {
                    mSynthPart.setSustain(event.value);
                }
                break;
                
            case EventType::SET_SYNTH_RELEASE:
                if (partIndex == 8) {
                    mSynthPart.setRelease(event.value);
                }
                break;
                
            case EventType::LOAD_SAMPLE:
                break;
                
            default:
                break;
        }
    }
}

oboe::DataCallbackResult AudioEngine::onAudioReady(
    oboe::AudioStream *stream,
    void *audioData,
    int32_t numFrames) {
    // Safety check: validate numFrames
    if (numFrames <= 0 || numFrames > MAX_FRAMES) {
        // Invalid frame count - return silence
        if (stream->getFormat() == oboe::AudioFormat::Float) {
            auto *outputBuffer = static_cast<float *>(audioData);
            int32_t numChannels = stream->getChannelCount();
            int32_t totalSamples = numFrames * numChannels;
            for (int32_t i = 0; i < totalSamples; i++) {
                outputBuffer[i] = 0.0f;
            }
        }
        return oboe::DataCallbackResult::Continue;
    }

    // Process all pending events first (realtime-critical)
    processEvents();

    // Get stream info (cached locally for performance)
    int32_t numChannels = stream->getChannelCount();
    double sampleRate = stream->getSampleRate();
    auto format = stream->getFormat();
    
    // Safety check: validate stream parameters
    if (numChannels <= 0 || numChannels > 8 || sampleRate <= 0.0 || sampleRate > 192000.0) {
        // Invalid stream parameters - return silence
        if (format == oboe::AudioFormat::Float) {
            auto *outputBuffer = static_cast<float *>(audioData);
            int32_t totalSamples = numFrames * numChannels;
            for (int32_t i = 0; i < totalSamples; i++) {
                outputBuffer[i] = 0.0f;
            }
        }
        return oboe::DataCallbackResult::Continue;
    }

    // M2 NEU: Update sequencer and get triggers
    Tribex::StepTrigger triggers[MAX_TRIGGERS_PER_CALLBACK];
    uint32_t numTriggers = 0;
    
    // Increment sample counter for this callback
    // Safety: clamp numFrames to prevent overflow
    int32_t safeNumFrames = (numFrames < 0) ? 0 : (numFrames > MAX_FRAMES ? MAX_FRAMES : numFrames);
    int64_t currentSample = mSampleCounter.fetch_add(safeNumFrames, std::memory_order_relaxed);
    
    // Safety check: prevent negative sample counter (shouldn't happen, but defensive)
    if (currentSample < 0) {
        mSampleCounter.store(0, std::memory_order_relaxed);
        currentSample = 0;
    }
    
    // Update sequencer (get trigger events)
    mSequencer.update(currentSample, sampleRate, triggers, &numTriggers);
    
    // Safety check: clamp numTriggers to prevent array overflow
    if (numTriggers > MAX_TRIGGERS_PER_CALLBACK) {
        numTriggers = MAX_TRIGGERS_PER_CALLBACK;
    }
    
    // M5: Process triggers and play voices (drum + synth)
    for (uint32_t i = 0; i < numTriggers; i++) {
        if (triggers[i].triggered) {
            // Trigger voice for this part
            uint32_t partIndex = triggers[i].partIndex;
            if (partIndex < Tribex::NUM_PARTS) {
                float velocity = Tribex::velocityToFloat(triggers[i].velocity);
                if (partIndex < 8) {
                    mSampleParts[partIndex].trigger(velocity);
                } else if (partIndex == 8) {
                    mSynthPart.trigger(velocity);
                }
            } else {
            }
        }
    }

    if (format == oboe::AudioFormat::Float) {
        auto *outputBuffer = static_cast<float *>(audioData);
        
        // P0.2: Use preallocated buffers for FX chain (no stack arrays!)
        // Use safeNumFrames already calculated above
        // Clear output buffers
        for (int32_t i = 0; i < safeNumFrames; i++) {
            mFXLeftBuffer[i] = 0.0f;
            mFXRightBuffer[i] = 0.0f;
        }
        
        // M4.5: Check if any part is soloed
        bool anyPartSoloed = mAnyPartSoloed.load(std::memory_order_relaxed);
        
        // M5: Render all drum parts (0-7)
        for (uint32_t part = 0; part < 8; part++) {
            // M4.5: Solo logic - only render if:
            // - No part is soloed AND part is not muted, OR
            // - This part is soloed AND not muted
            bool partMuted = mSampleParts[part].isMuted();
            bool partSoloed = mSampleParts[part].isSoloed();
            
            if (!partMuted && (!anyPartSoloed || partSoloed)) {
                if (mSampleParts[part].hasSample()) {
                    // Render to preallocated buffers (M4.5: no VLAs!)
                    // Clear buffers
                    for (int32_t i = 0; i < safeNumFrames; i++) {
                        mPartLeftBuffer[i] = 0.0f;
                        mPartRightBuffer[i] = 0.0f;
                    }
                    
                    mSampleParts[part].render(mPartLeftBuffer.data(), mPartRightBuffer.data(), safeNumFrames);
                    
                    // Sum to FX buffers (pre-FX buffers)
                    for (int32_t i = 0; i < safeNumFrames; i++) {
                        mFXLeftBuffer[i] += mPartLeftBuffer[i];
                        mFXRightBuffer[i] += mPartRightBuffer[i];
                    }
                }
            }
        }
        
        // M5: Render synth part (Part 8)
        // M4.5: Solo logic - only render if:
        // - No part is soloed AND part is not muted, OR
        // - This part is soloed AND not muted
        bool synthMuted = mSynthPart.isMuted();
        bool synthSoloed = mSynthPart.isSoloed();
        
        if (!synthMuted && (!anyPartSoloed || synthSoloed)) {
            if (mSynthPart.isPlaying()) {
                // Clear buffers
                for (int32_t i = 0; i < safeNumFrames; i++) {
                    mSynthLeftBuffer[i] = 0.0f;
                    mSynthRightBuffer[i] = 0.0f;
                }
                
                mSynthPart.render(mSynthLeftBuffer.data(), mSynthRightBuffer.data(), safeNumFrames);
                
                // Sum to FX buffers (pre-FX buffers)
                for (int32_t i = 0; i < safeNumFrames; i++) {
                    mFXLeftBuffer[i] += mSynthLeftBuffer[i];
                    mFXRightBuffer[i] += mSynthRightBuffer[i];
                }
            }
        }
        
        // M6: Process FX Chain (Delay � Reverb � Valve � Limiter)
        mFXManager.process(mFXLeftBuffer.data(), mFXRightBuffer.data(), 
                          mFXLeftBuffer.data(), mFXRightBuffer.data(), safeNumFrames);
        
        // M6: Copy processed output to interleaved outputBuffer
        for (int32_t i = 0; i < safeNumFrames; i++) {
            if (numChannels == 1) {
                outputBuffer[i] = mFXLeftBuffer[i];
            } else {
                int32_t base = i * numChannels;
                outputBuffer[base] = mFXLeftBuffer[i];
                outputBuffer[base + 1] = mFXRightBuffer[i];
                for (int32_t ch = 2; ch < numChannels; ch++) {
                    outputBuffer[base + ch] = 0.0f;
                }
            }
        }
        
        // M5: Keep test tone for debugging (optional, comment out for production)
        // generateSine(outputBuffer, numFrames, numChannels);
    } else {
        // Fallback to I16 if needed (shouldn't happen with our config)
        auto *outputBuffer = static_cast<int16_t *>(audioData);
        
        // Generate to float temp buffer first, then convert
        // P0.1: Use preallocated buffer instead of VLA
        // Use safeNumFrames already calculated above
        const int32_t bufferSize = safeNumFrames * numChannels;
        if (bufferSize > MAX_I16_BUFFER_SIZE) {
            // Safety check: should never happen with our config
            return oboe::DataCallbackResult::Stop;
        }
        
        generateSine(mI16TempBuffer.data(), safeNumFrames, numChannels);
        
        // Convert float to int16
        for (int32_t i = 0; i < bufferSize; i++) {
            float clamped = std::max(-1.0f, std::min(1.0f, mI16TempBuffer[i]));
            outputBuffer[i] = static_cast<int16_t>(clamped * 32767.0f);
        }
    }

    return oboe::DataCallbackResult::Continue;
}

void AudioEngine::onErrorAfterClose(oboe::AudioStream *stream, oboe::Result result) {
    mIsPlaying.store(false);
    mSequencer.stop();
}

oboe::Result AudioEngine::openStream() {
    oboe::AudioStreamBuilder builder;
    
    builder.setDirection(oboe::Direction::Output);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(oboe::SharingMode::Exclusive);
    builder.setFormat(oboe::AudioFormat::Float);
    builder.setChannelCount(oboe::ChannelCount::Stereo);
    builder.setDataCallback(this);
    builder.setErrorCallback(this);
    
    // Use AAudio on API 26+, fallback to OpenSL ES automatically
    return builder.openManagedStream(mStream);
}

oboe::Result AudioEngine::closeStream() {
    if (mStream) {
        return mStream->close();
    }
    return oboe::Result::OK;
}

void AudioEngine::generateSine(float *outputBuffer, int32_t numFrames, int32_t numChannels) {
    // CRITICAL: This is called from audio thread - NO ALLOCATIONS!
    
    // Load parameters atomically (latest values from UI events)
    float masterGain = mMasterGain.load(std::memory_order_relaxed);
    float masterPan = mMasterPan.load(std::memory_order_relaxed);
    float frequency = mTestToneFrequency.load(std::memory_order_relaxed);
    
    double sampleRate = mStream->getSampleRate();
    double phaseIncrement = (kTwoPi * frequency) / sampleRate;
    double phase = mPhase.load();
    
    // Calculate stereo pan values (constant power panning)
    float leftGain = std::sqrt(0.5f * (1.0f + masterPan));
    float rightGain = std::sqrt(0.5f * (1.0f - masterPan));
    
    for (int32_t i = 0; i < numFrames; i++) {
        // Generate sample value
        float sample = static_cast<float>(std::sin(phase));
        
        // Apply master gain
        sample *= masterGain;
        
        // Write to channels with stereo pan
        if (numChannels >= 2) {
            outputBuffer[i * numChannels] = sample * leftGain;
            outputBuffer[i * numChannels + 1] = sample * rightGain;
        } else {
            // Mono fallback
            outputBuffer[i] = sample * masterGain;
        }
        
        // Advance phase
        phase += phaseIncrement;
        
        // Wrap phase to prevent precision loss over long runs
        if (phase >= kTwoPi) {
            phase -= kTwoPi;
        }
    }
    
    // Store phase for next callback (atomic store)
    mPhase.store(phase);
}

// M6: FX Control methods
void AudioEngine::setDelayTimeMs(float timeMs) {
    mFXManager.setDelayTimeMs(timeMs);
}

void AudioEngine::setDelayFeedback(float feedback) {
    mFXManager.setDelayFeedback(feedback);
}

void AudioEngine::setDelayMix(float mix) {
    mFXManager.setDelayMix(mix);
}

void AudioEngine::setReverbSize(float size) {
    mFXManager.setReverbSize(size);
}

void AudioEngine::setReverbDensity(float density) {
    mFXManager.setReverbDensity(density);
}

void AudioEngine::setReverbMix(float mix) {
    mFXManager.setReverbMix(mix);
}

void AudioEngine::setValveAmount(float amount) {
    mFXManager.setValveAmount(amount);
}

void AudioEngine::setLimiterThresholdDb(float thresholdDb) {
    mFXManager.setLimiterThresholdDb(thresholdDb);
}

void AudioEngine::setLimiterReleaseMs(float releaseMs) {
    mFXManager.setLimiterReleaseMs(releaseMs);
}

// M6: Degradation control
void AudioEngine::setDegradationLevel(int level) {
    if (level < 0) level = 0;
    if (level > 2) level = 2;
    
    Tribex::DegradationLevel degradLevel = static_cast<Tribex::DegradationLevel>(level);
    mFXManager.setDegradationLevel(degradLevel);
    
    // Update max voices for sample parts
    int32_t maxVoices = mFXManager.getMaxVoices();
    for (int32_t i = 0; i < 8; i++) {
        mSampleParts[i].setMaxVoices(maxVoices);
    }
}

int AudioEngine::getDegradationLevel() const {
    return static_cast<int>(mFXManager.getDegradationLevel());
}

int32_t AudioEngine::getMaxVoices() const {
    return mFXManager.getMaxVoices();
}

void AudioEngine::resetXRunCounter() {
    mFXManager.resetXRunCounter();
}
