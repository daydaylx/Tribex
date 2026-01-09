#include "SamplePart.h"
#include <cstring>
#include <algorithm>

namespace Tribex {

SamplePart::SamplePart(uint32_t partIndex)
    : mPartIndex(partIndex)
    , mRetiredWriteIndex(0)
    , mRetiredReadIndex(0)
    , mSampleLoaded(false)
    , mSampleId(0)
    , mStartOffset(0)
    , mEndOffset(0)
    , mSampleDataPtr(nullptr)
    , mMuted(false)
    , mSoloed(false)
    , mFrameCounter(0)
    , mMaxVoices(MAX_VOICES_PER_PART)
{
    // Initialize voice start frames
    for (uint32_t i = 0; i < MAX_VOICES_PER_PART; i++) {
        mVoiceStartFrames[i] = 0;
    }
    
    // Initialize retired samples buffer
    for (uint32_t i = 0; i < MAX_RETIRED_SAMPLES; i++) {
        mRetiredSamples[i] = nullptr;
    }
}

SamplePart::~SamplePart() {
    unloadSample(false);
    releaseRetiredSamples();
}

void SamplePart::loadSample(const SampleData& sample, bool deferFree) {
    // Unload previous sample
    unloadSample(deferFree);
    
    // Copy sample metadata
    mSample.id = sample.id;
    mSample.sampleRate = sample.sampleRate;
    mSample.startOffset = sample.startOffset;
    mSample.endOffset = sample.endOffset;
    
    // Calculate effective length (with trim), using safe bounds
    uint32_t safeStart = std::min(sample.startOffset, sample.length);
    uint32_t safeEnd = sample.endOffset == 0 ? sample.length
                                             : std::min(sample.endOffset, sample.length);
    if (safeEnd <= safeStart) {
        safeStart = 0;
        safeEnd = sample.length;
    }
    
    uint32_t effectiveLength = safeEnd - safeStart;
    mSample.length = effectiveLength;
    mSample.startOffset = safeStart;
    mSample.endOffset = (safeEnd == sample.length) ? 0 : safeEnd;
    
    // Allocate sample data (float buffer)
    // Note: sample.data is jbyte* from JNI (containing float32 data)
    // We need to cast to read-only for memcpy
    if (sample.data != nullptr && effectiveLength > 0) {
        mSample.data = new (std::nothrow) float[effectiveLength];
        if (mSample.data != nullptr) {
            // Copy sample data (with trim)
            // sample.data is jbyte* containing float32 data
            uint32_t srcOffset = safeStart * sizeof(float);
            const uint8_t* srcBytes = reinterpret_cast<const uint8_t*>(sample.data);
            std::memcpy(mSample.data, srcBytes + srcOffset, 
                       effectiveLength * sizeof(float));
            mSample.loaded = true;
            
            // Store pointer atomically BEFORE setting loaded flag
            mSampleDataPtr.store(mSample.data, std::memory_order_release);
            mSampleId.store(sample.id, std::memory_order_release);
            mStartOffset.store(safeStart, std::memory_order_release);
            mEndOffset.store(safeEnd, std::memory_order_release);
            
            // Memory barrier: ensure all sample data is visible before setting flag
            std::atomic_thread_fence(std::memory_order_release);
            mSampleLoaded.store(true, std::memory_order_release);
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

void SamplePart::unloadSample(bool deferFree) {
    if (mSample.data != nullptr) {
        if (deferFree) {
            // Lock-free push to retired samples ring buffer
            uint32_t writeIdx = mRetiredWriteIndex.load(std::memory_order_acquire);
            uint32_t nextWriteIdx = (writeIdx + 1) % MAX_RETIRED_SAMPLES;
            uint32_t readIdx = mRetiredReadIndex.load(std::memory_order_acquire);
            
            // Check if buffer is full (with one slot reserved)
            if (nextWriteIdx != readIdx) {
                mRetiredSamples[writeIdx] = mSample.data;
                mRetiredWriteIndex.store(nextWriteIdx, std::memory_order_release);
            } else {
                // Buffer full - delete immediately (should rarely happen)
                delete[] mSample.data;
            }
        } else {
            delete[] mSample.data;
        }
        mSample.data = nullptr;
        mSampleDataPtr.store(nullptr, std::memory_order_release);
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

void SamplePart::releaseRetiredSamples() {
    // Lock-free pop from retired samples ring buffer
    uint32_t readIdx = mRetiredReadIndex.load(std::memory_order_acquire);
    uint32_t writeIdx = mRetiredWriteIndex.load(std::memory_order_acquire);
    
    // Process all retired samples in the queue
    while (readIdx != writeIdx) {
        float* data = mRetiredSamples[readIdx];
        if (data != nullptr) {
            delete[] data;
            mRetiredSamples[readIdx] = nullptr;
        }
        
        readIdx = (readIdx + 1) % MAX_RETIRED_SAMPLES;
        mRetiredReadIndex.store(readIdx, std::memory_order_release);
        
        // Reload write index in case new items were added
        writeIdx = mRetiredWriteIndex.load(std::memory_order_acquire);
    }
}

void SamplePart::setTrim(uint32_t startOffset, uint32_t endOffset) {
    if (mSample.data == nullptr) return;
    
    // Validate bounds
    if (startOffset > mSample.length) startOffset = mSample.length;
    if (endOffset == 0 || endOffset > mSample.length) endOffset = mSample.length;
    if (endOffset <= startOffset) {
        startOffset = 0;
        endOffset = mSample.length;
    }
    
    mStartOffset.store(startOffset, std::memory_order_relaxed);
    mEndOffset.store(endOffset, std::memory_order_relaxed);
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
    
    // Get current trim range
    uint32_t start = mStartOffset.load(std::memory_order_relaxed);
    uint32_t end = mEndOffset.load(std::memory_order_relaxed);
    uint32_t length = (end > start) ? (end - start) : 0;
    
    if (idleVoice != nullptr) {
        // Found idle voice - use it
        idleVoice->start(mSample.data + start, length, mSample.sampleRate, 
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
        
        oldestVoice->start(mSample.data + start, length, mSample.sampleRate, 
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
    uint32_t maxVoices = mMaxVoices.load(std::memory_order_relaxed);
    uint32_t safeMaxVoices = (maxVoices > MAX_VOICES_PER_PART) ? MAX_VOICES_PER_PART : maxVoices;
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

    uint32_t maxVoices = mMaxVoices.load(std::memory_order_relaxed);
    for (uint32_t i = 0; i < maxVoices; i++) {
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
    uint32_t maxVoices = mMaxVoices.load(std::memory_order_relaxed);
    for (uint32_t i = 0; i < maxVoices; i++) {
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
    
    mMaxVoices.store(maxVoices, std::memory_order_relaxed);
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
