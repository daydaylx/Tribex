#include "SamplePart.h"
#include <cstring>
#include <algorithm>

namespace Tribex {

SamplePart::SamplePart(uint32_t partIndex)
    : mPartIndex(partIndex)
    , mSampleLoaded(false)
    , mSampleId(0)
    , mMuted(false)
    , mSoloed(false)
    , mFrameCounter(0)
    , mMaxVoices(MAX_VOICES_PER_PART)
{
    // Initialize voice start frames
    for (uint32_t i = 0; i < MAX_VOICES_PER_PART; i++) {
        mVoiceStartFrames[i] = 0;
    }
}

SamplePart::~SamplePart() {
    unloadSample();
}

void SamplePart::loadSample(const SampleData& sample) {
    // Unload previous sample
    unloadSample();
    
    // Copy sample metadata
    mSample.id = sample.id;
    mSample.sampleRate = sample.sampleRate;
    mSample.startOffset = sample.startOffset;
    mSample.endOffset = sample.endOffset;
    
    // Calculate effective length (with trim)
    uint32_t effectiveLength = sample.length;
    if (sample.endOffset > 0) {
        effectiveLength = std::min(effectiveLength, sample.endOffset);
    }
    if (sample.startOffset > 0) {
        effectiveLength -= sample.startOffset;
    }
    
    mSample.length = effectiveLength;
    
    // Allocate sample data (float buffer)
    // Note: sample.data is jbyte* from JNI (containing float32 data)
    // We need to cast to read-only for memcpy
    if (sample.data != nullptr && effectiveLength > 0) {
        mSample.data = new (std::nothrow) float[effectiveLength];
        if (mSample.data != nullptr) {
            // Copy sample data (with trim)
            // sample.data is jbyte* containing float32 data
            uint32_t srcOffset = sample.startOffset * sizeof(float);
            const uint8_t* srcBytes = reinterpret_cast<const uint8_t*>(sample.data);
            std::memcpy(mSample.data, srcBytes + srcOffset, 
                       effectiveLength * sizeof(float));
            mSample.loaded = true;
            mSampleLoaded.store(true, std::memory_order_release);
            mSampleId.store(sample.id, std::memory_order_release);
        } else {
            mSample.loaded = false;
            mSampleLoaded.store(false, std::memory_order_release);
        }
    } else {
        mSample.data = nullptr;
        mSample.loaded = false;
        mSampleLoaded.store(false, std::memory_order_release);
    }
}

void SamplePart::unloadSample() {
    if (mSample.data != nullptr) {
        delete[] mSample.data;
        mSample.data = nullptr;
    }
    
    mSample.length = 0;
    mSample.loaded = false;
    mSampleLoaded.store(false, std::memory_order_release);
    mSampleId.store(0, std::memory_order_release);
    
    // Stop all voices
    for (uint32_t i = 0; i < MAX_VOICES_PER_PART; i++) {
        mVoices[i].stop();
    }
}

void SamplePart::setPitch(float pitch) {
    for (uint32_t i = 0; i < MAX_VOICES_PER_PART; i++) {
        mVoices[i].setPitch(pitch);
    }
}

void SamplePart::setPan(float pan) {
    for (uint32_t i = 0; i < MAX_VOICES_PER_PART; i++) {
        mVoices[i].setPan(pan);
    }
}

void SamplePart::setLevel(float level) {
    for (uint32_t i = 0; i < MAX_VOICES_PER_PART; i++) {
        mVoices[i].setLevel(level);
    }
}

void SamplePart::setDecay(float decayMs) {
    for (uint32_t i = 0; i < MAX_VOICES_PER_PART; i++) {
        mVoices[i].setDecay(decayMs);
    }
}

void SamplePart::setFilter(FilterType filter) {
    for (uint32_t i = 0; i < MAX_VOICES_PER_PART; i++) {
        mVoices[i].setFilter(filter);
    }
}

bool SamplePart::trigger(float velocity) {
    // Check if sample is loaded
    if (!mSampleLoaded.load(std::memory_order_acquire)) {
        return false;
    }
    
    // Check mute state (M4: UI only, no audio effect)
    if (mMuted.load(std::memory_order_acquire)) {
        return false;
    }
    
    // Find idle voice
    SampleVoice* idleVoice = findIdleVoice();
    
    if (idleVoice != nullptr) {
        // Found idle voice - use it
        idleVoice->start(mSample.data, mSample.length, mSample.sampleRate, 
                        velocity);
        
        // Update voice start frame for stealing
        uint32_t voiceIndex = idleVoice - mVoices;
        mVoiceStartFrames[voiceIndex] = mFrameCounter.fetch_add(1, std::memory_order_relaxed);
        
        return true;
    }
    
    // No idle voice - steal oldest
    SampleVoice* oldestVoice = findOldestVoice();
    
    if (oldestVoice != nullptr) {
        // Stop oldest voice and restart
        oldestVoice->stop();
        
        oldestVoice->start(mSample.data, mSample.length, mSample.sampleRate, 
                        velocity);
        
        // Update voice start frame for stealing
        uint32_t voiceIndex = oldestVoice - mVoices;
        mVoiceStartFrames[voiceIndex] = mFrameCounter.fetch_add(1, std::memory_order_relaxed);
        
        return true;
    }
    
    return false;
}

void SamplePart::render(float* leftBuffer, float* rightBuffer, int32_t numFrames) {
    // Safety check: null pointer validation
    if (!leftBuffer || !rightBuffer) {
        return;
    }
    
    // Safety check: validate numFrames
    if (numFrames <= 0 || numFrames > 1024) {
        return;
    }
    
    // Check if sample is loaded
    if (!mSampleLoaded.load(std::memory_order_acquire)) {
        return;
    }
    
    // Check mute state (M4: UI only, no audio effect)
    if (mMuted.load(std::memory_order_acquire)) {
        return;
    }
    
    // Safety check: validate mMaxVoices
    uint32_t safeMaxVoices = (mMaxVoices > MAX_VOICES_PER_PART) ? MAX_VOICES_PER_PART : mMaxVoices;
    if (safeMaxVoices == 0) {
        safeMaxVoices = 1;  // At least 1 voice
    }
    
    // Render each voice and sum to output buffers
    for (uint32_t i = 0; i < safeMaxVoices; i++) {
        if (mVoices[i].isPlaying()) {
            for (int32_t frame = 0; frame < numFrames; frame++) {
                float left, right;
                mVoices[i].renderSample(left, right);
                
                // Sum to output
                leftBuffer[frame] += left;
                rightBuffer[frame] += right;
            }
        }
    }
}

SampleVoice* SamplePart::findOldestVoice() {
    SampleVoice* oldest = nullptr;
    uint64_t oldestStart = UINT64_MAX;
    
    for (uint32_t i = 0; i < mMaxVoices; i++) {
        if (mVoices[i].isPlaying()) {
            if (mVoiceStartFrames[i] < oldestStart) {
                oldestStart = mVoiceStartFrames[i];
                oldest = &mVoices[i];
            }
        }
    }
    
    return oldest;
}

SampleVoice* SamplePart::findIdleVoice() {
    for (uint32_t i = 0; i < mMaxVoices; i++) {
        if (!mVoices[i].isPlaying()) {
            return &mVoices[i];
        }
    }
    return nullptr;
}

void SamplePart::setMaxVoices(uint32_t maxVoices) {
    // Clamp to valid range (1 to MAX_VOICES_PER_PART)
    if (maxVoices < 1) maxVoices = 1;
    if (maxVoices > MAX_VOICES_PER_PART) maxVoices = MAX_VOICES_PER_PART;
    
    mMaxVoices = maxVoices;
}

uint32_t SamplePart::getActiveVoiceCount() const {
    uint32_t count = 0;
    for (uint32_t i = 0; i < MAX_VOICES_PER_PART; i++) {
        if (mVoices[i].isPlaying()) {
            count++;
        }
    }
    return count;
}

} // namespace Tribex