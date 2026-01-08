#include <jni.h>
#include "AudioEngine.h"
#include <android/log.h>
#include <memory>

#define TAG "NativeLib"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// Global audio engine instance
// Note: In production, this should be managed more carefully
std::unique_ptr<AudioEngine> gAudioEngine;

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_tribex_groovebox_MainActivity_nativeStartAudioEngine(JNIEnv *env, jobject thiz) {
    LOGI("nativeStartAudioEngine called");
    
    if (!gAudioEngine) {
        gAudioEngine = std::make_unique<AudioEngine>();
    }
    
    return static_cast<jboolean>(gAudioEngine->start());
}

JNIEXPORT jboolean JNICALL
Java_com_tribex_groovebox_MainActivity_nativeStopAudioEngine(JNIEnv *env, jobject thiz) {
    LOGI("nativeStopAudioEngine called");
    
    if (!gAudioEngine) {
        return JNI_TRUE; // Already stopped
    }
    
    return static_cast<jboolean>(gAudioEngine->stop());
}

JNIEXPORT jboolean JNICALL
Java_com_tribex_groovebox_MainActivity_nativeIsPlaying(JNIEnv *env, jobject thiz) {
    if (!gAudioEngine) {
        return JNI_FALSE;
    }
    
    return static_cast<jboolean>(gAudioEngine->isPlaying());
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_MainActivity_nativeCleanup(JNIEnv *env, jobject thiz) {
    LOGI("nativeCleanup called");
    
    if (gAudioEngine) {
        gAudioEngine->stop();
        gAudioEngine.reset();
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_MainActivity_toggleNativeAudio(JNIEnv *env, jobject thiz) {
    LOGI("toggleNativeAudio called");
    
    if (!gAudioEngine) {
        gAudioEngine = std::make_unique<AudioEngine>();
    }
    
    if (gAudioEngine->isPlaying()) {
        gAudioEngine->stop();
    } else {
        gAudioEngine->start();
    }
}

JNIEXPORT jboolean JNICALL
Java_com_tribex_groovebox_MainActivity_isNativePlaying(JNIEnv *env, jobject thiz) {
    if (!gAudioEngine) {
        return JNI_FALSE;
    }
    
    return static_cast<jboolean>(gAudioEngine->isPlaying());
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_MainActivity_nativeSetMasterGain(JNIEnv *env, jobject thiz, jfloat gain) {
    if (gAudioEngine) {
        gAudioEngine->setMasterGain(gain);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_MainActivity_nativeSetMasterPan(JNIEnv *env, jobject thiz, jfloat pan) {
    if (gAudioEngine) {
        gAudioEngine->setMasterPan(pan);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_MainActivity_nativeSetTestToneFrequency(JNIEnv *env, jobject thiz, jfloat freq) {
    if (gAudioEngine) {
        gAudioEngine->setTestToneFrequency(freq);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_MainActivity_nativeClearEvents(JNIEnv *env, jobject thiz) {
    if (gAudioEngine) {
        gAudioEngine->clearEvents();
    }
}

// M2 NEU: Sequencer Control Methods

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetBPM(JNIEnv *env, jobject thiz, jfloat bpm) {
    if (gAudioEngine) {
        gAudioEngine->setBPM(bpm);
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeStartSequencer(JNIEnv *env, jobject thiz) {
    if (gAudioEngine) {
        gAudioEngine->startSequencer();
    }
}

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeStopSequencer(JNIEnv *env, jobject thiz) {
    if (gAudioEngine) {
        gAudioEngine->stopSequencer();
    }
}

JNIEXPORT jboolean JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeIsSequencerPlaying(JNIEnv *env, jobject thiz) {
    if (!gAudioEngine) {
        return JNI_FALSE;
    }
    
    return static_cast<jboolean>(gAudioEngine->isSequencerPlaying());
}

// M4 NEU: Sample Engine Control Methods

JNIEXPORT void JNICALL
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeLoadSample(JNIEnv *env, jobject thiz, 
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeUnloadSample(JNIEnv *env, jobject thiz, jint partIndex) {
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetVoicePitch(JNIEnv *env, jobject thiz, 
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetVoicePan(JNIEnv *env, jobject thiz, 
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetVoiceLevel(JNIEnv *env, jobject thiz, 
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetVoiceDecay(JNIEnv *env, jobject thiz, 
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetVoiceFilter(JNIEnv *env, jobject thiz, 
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetPartMute(JNIEnv *env, jobject thiz, 
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetPartSolo(JNIEnv *env, jobject thiz, 
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetSynthWavetable(JNIEnv *env, jobject thiz, 
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetSynthCutoff(JNIEnv *env, jobject thiz, 
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetSynthResonance(JNIEnv *env, jobject thiz, 
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetSynthAttack(JNIEnv *env, jobject thiz, 
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetSynthDecay(JNIEnv *env, jobject thiz, 
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetSynthSustain(JNIEnv *env, jobject thiz, 
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
Java_com_tribex_groovebox_engine_AudioEngineBridge_nativeSetSynthRelease(JNIEnv *env, jobject thiz, 
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

} // extern "C"