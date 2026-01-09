#include <jni.h>
#include "AudioEngine.h"
#include "OfflineRenderer.h"
#include <android/log.h>
#include <memory>
#include <string>

#define TAG "NativeLib"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// Global audio engine instance
std::unique_ptr<AudioEngine> gAudioEngine;

// Global offline renderer instance
std::unique_ptr<Tribex::OfflineRenderer> gOfflineRenderer;

extern "C" {

// AudioEngineBridge: Core engine control methods

JNIEXPORT jboolean JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_startAudioEngine(JNIEnv *env, jobject thiz) {
    if (!gAudioEngine) {
        gAudioEngine = std::make_unique<AudioEngine>();
    }

    return static_cast<jboolean>(gAudioEngine->start());
}

JNIEXPORT jboolean JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_stopAudioEngine(JNIEnv *env, jobject thiz) {
    if (!gAudioEngine) {
        return JNI_TRUE;
    }

    return static_cast<jboolean>(gAudioEngine->stop());
}

JNIEXPORT jboolean JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_isPlaying(JNIEnv *env, jobject thiz) {
    if (!gAudioEngine) {
        return JNI_FALSE;
    }

    return static_cast<jboolean>(gAudioEngine->isPlaying());
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_toggleNativeAudio(JNIEnv *env, jobject thiz) {
    if (!gAudioEngine) {
        gAudioEngine = std::make_unique<AudioEngine>();
    }

    if (gAudioEngine->isPlaying()) {
        gAudioEngine->stop();
    } else {
        gAudioEngine->start();
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_cleanup(JNIEnv *env, jobject thiz) {
    if (gAudioEngine) {
        gAudioEngine->stop();
        gAudioEngine.reset();
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setMasterGain(JNIEnv *env, jobject thiz, jfloat gain) {
    if (gAudioEngine) {
        gAudioEngine->setMasterGain(gain);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setMasterPan(JNIEnv *env, jobject thiz, jfloat pan) {
    if (gAudioEngine) {
        gAudioEngine->setMasterPan(pan);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setTestToneFrequency(JNIEnv *env, jobject thiz, jfloat freq) {
    if (gAudioEngine) {
        gAudioEngine->setTestToneFrequency(freq);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_clearEvents(JNIEnv *env, jobject thiz) {
    if (gAudioEngine) {
        gAudioEngine->clearEvents();
    }
}

// M2 NEU: Sequencer Control Methods

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setBPM(JNIEnv *env, jobject thiz, jfloat bpm) {
    if (gAudioEngine) {
        gAudioEngine->setBPM(bpm);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_startSequencer(JNIEnv *env, jobject thiz) {
    if (gAudioEngine) {
        gAudioEngine->startSequencer();
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_stopSequencer(JNIEnv *env, jobject thiz) {
    if (gAudioEngine) {
        gAudioEngine->stopSequencer();
    }
}

JNIEXPORT jboolean JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_isSequencerPlaying(JNIEnv *env, jobject thiz) {
    if (!gAudioEngine) {
        return JNI_FALSE;
    }
    
    return static_cast<jboolean>(gAudioEngine->isSequencerPlaying());
}

// P0.4: Sequencer State Methods

JNIEXPORT jint JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_getCurrentStep(JNIEnv *env, jobject thiz) {
    if (!gAudioEngine) {
        return 0;
    }
    
    uint32_t step = gAudioEngine->getCurrentStep();
    
    return static_cast<jint>(step);
}

JNIEXPORT jint JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_getLoopIteration(JNIEnv *env, jobject thiz) {
    if (!gAudioEngine) {
        return 0;
    }
    
    return static_cast<jint>(gAudioEngine->getLoopIteration());
}

// Pattern Management

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setPattern(JNIEnv *env, jobject thiz,
                                                                jbyteArray patternData,
                                                                jint patternLength,
                                                                jint patternSeed) {
    if (!gAudioEngine) {
        LOGE("Audio engine not initialized");
        return;
    }
    
    // Validate pattern length
    if (patternLength != 16 && patternLength != 32 && patternLength != 48 && patternLength != 64) {
        LOGE("Invalid pattern length: %d (must be 16, 32, 48, or 64)", patternLength);
        return;
    }
    
    // Get pattern data
    jsize dataLength = env->GetArrayLength(patternData);
    jbyte* data = env->GetByteArrayElements(patternData, nullptr);
    if (data == nullptr) {
        LOGE("Failed to get pattern data array");
        return;
    }
    
    // Convert to C++ Pattern structure
    // Pattern structure: id, lengthSteps, patternSeed, steps[9][64]
    // Serialized format (little-endian):
    // Header (12 bytes): id(4) + lengthSteps(4) + patternSeed(4)
    // Steps: NUM_PARTS * MAX_STEPS * 4 bytes each (gate(1) + velocity(1) + microtiming(1) + probability(1))
    
    Tribex::Pattern patternObj;
    
    // Parse header (first 12 bytes)
    if (dataLength >= 12) {
        // Parse id (uint32_t, little-endian) - cast to unsigned first to handle sign extension
        patternObj.id = static_cast<uint32_t>(static_cast<uint8_t>(data[0])) |
                        (static_cast<uint32_t>(static_cast<uint8_t>(data[1])) << 8) |
                        (static_cast<uint32_t>(static_cast<uint8_t>(data[2])) << 16) |
                        (static_cast<uint32_t>(static_cast<uint8_t>(data[3])) << 24);
        
        // Parse lengthSteps (uint32_t, little-endian)
        patternObj.lengthSteps = static_cast<uint32_t>(static_cast<uint8_t>(data[4])) |
                                 (static_cast<uint32_t>(static_cast<uint8_t>(data[5])) << 8) |
                                 (static_cast<uint32_t>(static_cast<uint8_t>(data[6])) << 16) |
                                 (static_cast<uint32_t>(static_cast<uint8_t>(data[7])) << 24);
        
        // Parse patternSeed (uint32_t, little-endian)
        patternObj.patternSeed = static_cast<uint32_t>(static_cast<uint8_t>(data[8])) |
                                 (static_cast<uint32_t>(static_cast<uint8_t>(data[9])) << 8) |
                                 (static_cast<uint32_t>(static_cast<uint8_t>(data[10])) << 16) |
                                 (static_cast<uint32_t>(static_cast<uint8_t>(data[11])) << 24);
        
        // Validate parsed lengthSteps matches parameter
        if (patternObj.lengthSteps != static_cast<uint32_t>(patternLength)) {
            patternObj.lengthSteps = static_cast<uint32_t>(patternLength);
        }
    } else {
        // Fallback if header is missing
        patternObj.id = 0;
        patternObj.lengthSteps = static_cast<uint32_t>(patternLength);
        patternObj.patternSeed = static_cast<uint32_t>(patternSeed);
    }
    
    // Initialize all steps to empty first
    for (uint32_t part = 0; part < Tribex::NUM_PARTS; part++) {
        for (uint32_t step = 0; step < Tribex::MAX_STEPS; step++) {
            patternObj.steps[part][step] = Tribex::StepData();
        }
    }
    
    // Parse step data (starts at offset 12)
    constexpr int32_t headerSize = 12;
    constexpr int32_t stepSize = 4; // gate(1) + velocity(1) + microtiming(1) + probability(1)
    int32_t expectedDataSize = headerSize + (Tribex::NUM_PARTS * Tribex::MAX_STEPS * stepSize);
    
    if (dataLength >= expectedDataSize) {
        int32_t offset = headerSize;
        
        for (uint32_t part = 0; part < Tribex::NUM_PARTS; part++) {
            for (uint32_t step = 0; step < Tribex::MAX_STEPS; step++) {
                if (offset + stepSize <= dataLength) {
                    patternObj.steps[part][step].gate = data[offset] != 0 ? 1 : 0;
                    patternObj.steps[part][step].velocity = static_cast<uint8_t>(data[offset + 1]) & 0x03; // 2-bit value
                    patternObj.steps[part][step].microtiming = static_cast<int8_t>(data[offset + 2]);
                    patternObj.steps[part][step].probability = static_cast<int8_t>(data[offset + 3]);
                    offset += stepSize;
                }
            }
        }
        
        // Count active steps for debugging
        uint32_t activeSteps = 0;
        for (uint32_t part = 0; part < Tribex::NUM_PARTS; part++) {
            for (uint32_t step = 0; step < patternObj.lengthSteps && step < Tribex::MAX_STEPS; step++) {
                if (patternObj.steps[part][step].gate != 0) {
                    activeSteps++;
                }
            }
        }
        
        LOGI("Pattern parsed: id=%u, length=%u, seed=%u, dataLength=%d, activeSteps=%u", 
             patternObj.id, patternObj.lengthSteps, patternObj.patternSeed, dataLength, activeSteps);
    } else {
        LOGE("Pattern data too short: %d bytes, expected at least %d bytes", dataLength, expectedDataSize);
    }
    
    env->ReleaseByteArrayElements(patternData, data, JNI_ABORT);
    
    // Set pattern in sequencer
    gAudioEngine->setPattern(patternObj);
    
    LOGI("Pattern set: length=%d, seed=%d", patternLength, patternSeed);
}

// M4 NEU: Sample Engine Control Methods

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_loadSample(JNIEnv *env, jobject thiz, 
                                                        jint partIndex,
                                                        jbyteArray sampleData,
                                                        jint length,
                                                        jint sampleRate,
                                                        jint sampleId,
                                                        jint startOffset,
                                                        jint endOffset) {
    if (!gAudioEngine) {
        LOGE("Audio engine not initialized");
        return;
    }
    
    if (partIndex < 0 || partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    // Get byte array pointer
    jbyte* data = env->GetByteArrayElements(sampleData, nullptr);
    if (data == nullptr) {
        LOGE("Failed to get sample data array");
        return;
    }
    
    // Cast to float (assume input is already float32)
    float* floatData = reinterpret_cast<float*>(data);
    
    // Create SampleData structure
    Tribex::SampleData sample;
    sample.data = floatData;
    
    // Safety check: ensure length is divisible by sizeof(float)
    if (length % sizeof(float) != 0) {
        LOGE("Sample length %d is not divisible by sizeof(float) %zu", length, sizeof(float));
        env->ReleaseByteArrayElements(sampleData, data, JNI_ABORT);
        return;
    }
    
    sample.length = static_cast<uint32_t>(length / sizeof(float));
    sample.sampleRate = static_cast<uint32_t>(sampleRate);
    sample.id = static_cast<uint32_t>(sampleId);
    sample.startOffset = static_cast<uint32_t>(startOffset);
    sample.endOffset = static_cast<uint32_t>(endOffset);
    
    // Load sample into audio engine
    gAudioEngine->loadSample(static_cast<uint32_t>(partIndex), sample);
    
    // Release byte array (but don't copy back - audio engine made a copy)
    env->ReleaseByteArrayElements(sampleData, data, JNI_ABORT);
    
    LOGI("Loaded sample %d into part %d", sampleId, partIndex);
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_unloadSample(JNIEnv *env, jobject thiz, jint partIndex) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex < 0 || partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    gAudioEngine->unloadSample(static_cast<uint32_t>(partIndex));
    
    LOGI("Unloaded sample from part %d", partIndex);
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setVoicePitch(JNIEnv *env, jobject thiz, 
                                                           jint partIndex, jfloat pitch) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex < 0 || partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    gAudioEngine->setVoicePitch(static_cast<uint32_t>(partIndex), pitch);
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setVoicePan(JNIEnv *env, jobject thiz, 
                                                         jint partIndex, jfloat pan) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex < 0 || partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    gAudioEngine->setVoicePan(static_cast<uint32_t>(partIndex), pan);
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setVoiceLevel(JNIEnv *env, jobject thiz, 
                                                           jint partIndex, jfloat level) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex < 0 || partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    gAudioEngine->setVoiceLevel(static_cast<uint32_t>(partIndex), level);
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setVoiceDecay(JNIEnv *env, jobject thiz, 
                                                           jint partIndex, jfloat decayMs) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex < 0 || partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    gAudioEngine->setVoiceDecay(static_cast<uint32_t>(partIndex), decayMs);
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setVoiceFilter(JNIEnv *env, jobject thiz, 
                                                            jint partIndex, jint filterType) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex < 0 || partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    Tribex::FilterType filter = (filterType == 1) 
        ? Tribex::FilterType::HP 
        : Tribex::FilterType::LP;
    
    gAudioEngine->setVoiceFilter(static_cast<uint32_t>(partIndex), filter);
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setVoiceTrim(JNIEnv *env, jobject thiz, 
                                                          jint partIndex, jint startOffset, jint endOffset) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex < 0 || partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    gAudioEngine->setVoiceTrim(static_cast<uint32_t>(partIndex), 
                              static_cast<uint32_t>(startOffset), 
                              static_cast<uint32_t>(endOffset));
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setPartMute(JNIEnv *env, jobject thiz, 
                                                         jint partIndex, jboolean muted) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex < 0 || partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    gAudioEngine->setPartMute(static_cast<uint32_t>(partIndex), muted == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setPartSolo(JNIEnv *env, jobject thiz, 
                                                         jint partIndex, jboolean solo) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex < 0 || partIndex >= Tribex::NUM_PARTS) {
        LOGE("Invalid part index: %d", partIndex);
        return;
    }
    
    gAudioEngine->setPartSolo(static_cast<uint32_t>(partIndex), solo == JNI_TRUE);
}

// M5 NEU: Synth Part Control Methods (Part 8 only)

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setSynthWavetable(JNIEnv *env, jobject thiz, 
                                                                   jint partIndex, jint wavetableType) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex != 8) {
        LOGE("Synth wavetable only valid for part 8, got: %d", partIndex);
        return;
    }
    
    gAudioEngine->setSynthWavetable(static_cast<uint32_t>(partIndex), static_cast<uint8_t>(wavetableType));
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setSynthCutoff(JNIEnv *env, jobject thiz, 
                                                                 jint partIndex, jfloat cutoff) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex != 8) {
        LOGE("Synth cutoff only valid for part 8, got: %d", partIndex);
        return;
    }
    
    gAudioEngine->setSynthCutoff(static_cast<uint32_t>(partIndex), cutoff);
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setSynthResonance(JNIEnv *env, jobject thiz, 
                                                                    jint partIndex, jfloat resonance) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex != 8) {
        LOGE("Synth resonance only valid for part 8, got: %d", partIndex);
        return;
    }
    
    gAudioEngine->setSynthResonance(static_cast<uint32_t>(partIndex), resonance);
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setSynthAttack(JNIEnv *env, jobject thiz, 
                                                                  jint partIndex, jfloat attackMs) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex != 8) {
        LOGE("Synth attack only valid for part 8, got: %d", partIndex);
        return;
    }
    
    gAudioEngine->setSynthAttack(static_cast<uint32_t>(partIndex), attackMs);
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setSynthDecay(JNIEnv *env, jobject thiz, 
                                                                  jint partIndex, jfloat decayMs) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex != 8) {
        LOGE("Synth decay only valid for part 8, got: %d", partIndex);
        return;
    }
    
    gAudioEngine->setSynthDecay(static_cast<uint32_t>(partIndex), decayMs);
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setSynthSustain(JNIEnv *env, jobject thiz, 
                                                                    jint partIndex, jfloat sustainLevel) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex != 8) {
        LOGE("Synth sustain only valid for part 8, got: %d", partIndex);
        return;
    }
    
    gAudioEngine->setSynthSustain(static_cast<uint32_t>(partIndex), sustainLevel);
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setSynthRelease(JNIEnv *env, jobject thiz, 
                                                                    jint partIndex, jfloat releaseMs) {
    if (!gAudioEngine) {
        return;
    }
    
    if (partIndex != 8) {
        LOGE("Synth release only valid for part 8, got: %d", partIndex);
        return;
    }
    
    gAudioEngine->setSynthRelease(static_cast<uint32_t>(partIndex), releaseMs);
}

// M6 NEU: FX Control Methods

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setDelayTimeMs(JNIEnv *env, jobject thiz, jfloat timeMs) {
    if (gAudioEngine) {
        gAudioEngine->setDelayTimeMs(timeMs);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setDelayFeedback(JNIEnv *env, jobject thiz, jfloat feedback) {
    if (gAudioEngine) {
        gAudioEngine->setDelayFeedback(feedback);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setDelayMix(JNIEnv *env, jobject thiz, jfloat mix) {
    if (gAudioEngine) {
        gAudioEngine->setDelayMix(mix);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setReverbSize(JNIEnv *env, jobject thiz, jfloat size) {
    if (gAudioEngine) {
        gAudioEngine->setReverbSize(size);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setReverbDensity(JNIEnv *env, jobject thiz, jfloat density) {
    if (gAudioEngine) {
        gAudioEngine->setReverbDensity(density);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setReverbMix(JNIEnv *env, jobject thiz, jfloat mix) {
    if (gAudioEngine) {
        gAudioEngine->setReverbMix(mix);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setValveAmount(JNIEnv *env, jobject thiz, jfloat amount) {
    if (gAudioEngine) {
        gAudioEngine->setValveAmount(amount);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setLimiterThresholdDb(JNIEnv *env, jobject thiz, jfloat thresholdDb) {
    if (gAudioEngine) {
        gAudioEngine->setLimiterThresholdDb(thresholdDb);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setLimiterReleaseMs(JNIEnv *env, jobject thiz, jfloat releaseMs) {
    if (gAudioEngine) {
        gAudioEngine->setLimiterReleaseMs(releaseMs);
    }
}

// M6 NEU: Degradation Control Methods

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_setDegradationLevel(JNIEnv *env, jobject thiz, jint level) {
    if (gAudioEngine) {
        gAudioEngine->setDegradationLevel(level);
    }
}

JNIEXPORT jint JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_getDegradationLevel(JNIEnv *env, jobject thiz) {
    if (!gAudioEngine) {
        return 0;
    }
    
    return gAudioEngine->getDegradationLevel();
}

JNIEXPORT jint JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_getMaxVoices(JNIEnv *env, jobject thiz) {
    if (!gAudioEngine) {
        return 24; // Default
    }
    
    return static_cast<jint>(gAudioEngine->getMaxVoices());
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_resetXRunCounter(JNIEnv *env, jobject thiz) {
    if (gAudioEngine) {
        gAudioEngine->resetXRunCounter();
    }
}

// P1.2: Export JNI Methods

JNIEXPORT jboolean JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_startExport(JNIEnv *env, jobject thiz, jstring filename) {
    if (!gAudioEngine) {
        LOGE("Audio engine not initialized");
        return JNI_FALSE;
    }
    
    if (!gOfflineRenderer) {
        gOfflineRenderer = std::make_unique<Tribex::OfflineRenderer>();
        gOfflineRenderer->setAudioEngine(gAudioEngine.get());
    }
    
    if (gOfflineRenderer->isExporting()) {
        LOGE("Export already in progress");
        return JNI_FALSE;
    }
    
    const char* cstr = env->GetStringUTFChars(filename, nullptr);
    if (cstr == nullptr) {
        LOGE("Failed to get filename string");
        return JNI_FALSE;
    }
    
    std::string filePath(cstr);
    env->ReleaseStringUTFChars(filename, cstr);
    
    bool result = gOfflineRenderer->startExport(filePath, 44100, nullptr);
    
    if (!result) {
        LOGE("Failed to start export: %s", gOfflineRenderer->getErrorMessage().c_str());
    }
    
    return static_cast<jboolean>(result);
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_stopExport(JNIEnv *env, jobject thiz) {
    if (gOfflineRenderer) {
        gOfflineRenderer->stopExport();
    }
}

JNIEXPORT jfloat JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_getExportProgress(JNIEnv *env, jobject thiz) {
    if (gOfflineRenderer) {
        return gOfflineRenderer->getProgress();
    }
    return 0.0f;
}

JNIEXPORT jboolean JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_isExporting(JNIEnv *env, jobject thiz) {
    if (!gOfflineRenderer) {
        return JNI_FALSE;
    }
    
    return static_cast<jboolean>(gOfflineRenderer->isExporting());
}

} // extern "C"
