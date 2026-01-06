#include "AudioEngine.h"
#include <android/log.h>
#include <cmath>
#include <cstring>

#define TAG "AudioEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

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
    , mParts{0, 1, 2, 3, 4, 5, 6, 7, 8}  // Initialize 9 parts (8 drums + 1 synth)
{
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

// M4: Sample Engine methods
void AudioEngine::loadSample(uint32_t partIndex, const Tribex::SampleData& sample) {
    if (partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    // Load sample into part (called from IO thread, allocations allowed)
    mParts[partIndex].loadSample(sample);
    
    LOGI("Loaded sample %d into part %d", sample.id, partIndex);
}

void AudioEngine::unloadSample(uint32_t partIndex) {
    if (partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    mParts[partIndex].unloadSample();
    
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

void AudioEngine::setPartMute(uint32_t partIndex, bool muted) {
    if (partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    mParts[partIndex].setMute(muted);
    
    LOGI("Part %d muted: %d", partIndex, muted);
}

void AudioEngine::setPartSolo(uint32_t partIndex, bool solo) {
    if (partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    mParts[partIndex].setSolo(solo);
    
    LOGI("Part %d soloed: %d", partIndex, solo);
}

void AudioEngine::processEvents() {
    // CRITICAL: This is called from audio thread - NO ALLOCATIONS!
    
    AudioEvent event;
    while (mEventQueue.pop(event)) {
        if (!event.isValid()) {
            LOGE("Invalid event received, skipping");
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
                    mParts[partIndex].trigger(velocity);
                }
                break;
                
            case EventType::SET_VOICE_PITCH:
                if (partIndex < Tribex::NUM_PARTS) {
                    mParts[partIndex].setPitch(event.value);
                }
                break;
                
            case EventType::SET_VOICE_PAN:
                if (partIndex < Tribex::NUM_PARTS) {
                    mParts[partIndex].setPan(event.value);
                }
                break;
                
            case EventType::SET_VOICE_LEVEL:
                if (partIndex < Tribex::NUM_PARTS) {
                    mParts[partIndex].setLevel(event.value);
                }
                break;
                
            case EventType::SET_VOICE_DECAY:
                if (partIndex < Tribex::NUM_PARTS) {
                    mParts[partIndex].setDecay(event.value);
                }
                break;
                
            case EventType::SET_VOICE_FILTER:
                if (partIndex < Tribex::NUM_PARTS) {
                    Tribex::FilterType filter = (event.value > 0.5f) 
                        ? Tribex::FilterType::HP 
                        : Tribex::FilterType::LP;
                    mParts[partIndex].setFilter(filter);
                }
                break;
                
            case EventType::LOAD_SAMPLE:
                // Sample loading is done via loadSample() method (IO thread)
                // This event is for future use
                break;
                
            default:
                // Reserved events - ignore
                break;
        }
    }
}

oboe::DataCallbackResult AudioEngine::onAudioReady(
    oboe::AudioStream *stream,
    void *audioData,
    int32_t numFrames) {

    // Process all pending events first (realtime-critical)
    processEvents();

    // Get stream info (cached locally for performance)
    int32_t numChannels = stream->getChannelCount();
    double sampleRate = stream->getSampleRate();
    auto format = stream->getFormat();

    // M2 NEU: Update sequencer and get triggers
    Tribex::StepTrigger triggers[MAX_TRIGGERS_PER_CALLBACK];
    uint32_t numTriggers = 0;
    
    // Increment sample counter for this callback
    int64_t currentSample = mSampleCounter.fetch_add(numFrames, std::memory_order_relaxed);
    
    // Update sequencer (get trigger events)
    mSequencer.update(currentSample, sampleRate, triggers, &numTriggers);
    
    // M4: Process triggers and play sample voices
    for (uint32_t i = 0; i < numTriggers; i++) {
        if (triggers[i].triggered) {
            // Trigger voice for this part
            uint32_t partIndex = triggers[i].partIndex;
            if (partIndex < Tribex::NUM_PARTS) {
                float velocity = Tribex::velocityToFloat(triggers[i].velocity);
                mParts[partIndex].trigger(velocity);
            }
        }
    }

    if (format == oboe::AudioFormat::Float) {
        auto *outputBuffer = static_cast<float *>(audioData);
        
        // M4: Clear output buffers
        for (int32_t i = 0; i < numFrames * numChannels; i++) {
            outputBuffer[i] = 0.0f;
        }
        
        // Render all sample parts
        // M4: Only drum parts (0-7), synth (8) not implemented yet
        for (uint32_t part = 0; part < 8; part++) {
            if (mParts[part].hasSample()) {
                // Render to temporary buffers then sum
                // Note: This is a simple approach, could be optimized
                float leftBuffer[numFrames];
                float rightBuffer[numFrames];
                
                for (int32_t i = 0; i < numFrames; i++) {
                    leftBuffer[i] = 0.0f;
                    rightBuffer[i] = 0.0f;
                }
                
                mParts[part].render(leftBuffer, rightBuffer, numFrames);
                
                // Sum to output
                for (int32_t i = 0; i < numFrames; i++) {
                    outputBuffer[i * numChannels] += leftBuffer[i];
                    outputBuffer[i * numChannels + 1] += rightBuffer[i];
                }
            }
        }
        
        // M4: Keep test tone for debugging (optional, comment out for production)
        // generateSine(outputBuffer, numFrames, numChannels);
    } else {
        // Fallback to I16 if needed (shouldn't happen with our config)
        auto *outputBuffer = static_cast<int16_t *>(audioData);
        
        // Generate to float temp buffer first, then convert
        // Note: This is a temporary buffer on stack - acceptable for M0
        const int32_t bufferSize = numFrames * numChannels;
        float tempBuffer[bufferSize];
        
        generateSine(tempBuffer, numFrames, numChannels);
        
        // Convert float to int16
        for (int32_t i = 0; i < bufferSize; i++) {
            float clamped = std::max(-1.0f, std::min(1.0f, tempBuffer[i]));
            outputBuffer[i] = static_cast<int16_t>(clamped * 32767.0f);
        }
    }

    return oboe::DataCallbackResult::Continue;
}

void AudioEngine::onErrorAfterClose(oboe::AudioStream *stream, oboe::Result result) {
    LOGE("Stream error: %s", oboe::convertToText(result));
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