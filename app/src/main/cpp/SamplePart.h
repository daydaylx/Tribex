#ifndef TRIBEX_SAMPLEPART_H
#define TRIBEX_SAMPLEPART_H

#include "SampleVoice.h"
#include <cstdint>
#include <atomic>
#include <vector>
#include <mutex>

namespace Tribex {

/**
 * Sample Part - Manages up to 4 voices for one drum part
 * 
 * This is called from audio thread - NO ALLOCATIONS!
 * 
 * Features:
 * - Voice Pool (max 4 voices)
 * - Voice Stealing (oldest voice gets stolen)
 * - Per-part parameters (pitch, pan, level, decay, filter)
 * - Sample data management (pointer to sample buffer)
 */

// Maximum voices per part
constexpr uint32_t MAX_VOICES_PER_PART = 4;

/**
 * Sample Data Structure
 * Contains sample data and metadata
 * NOTE: Sample data is owned by SamplePart, not by individual voices
 * IMPORTANT: data is passed as jbyte* from JNI (containing float32 data)
 * 
 * When used for loading from JNI: data is const (input)
 * When used in SamplePart: data is non-const (owned by part)
 */
struct SampleData {
    float* data;          // Audio data (float32, mono)
    uint32_t length;       // Length in samples
    uint32_t sampleRate;   // Original sample rate
    uint32_t id;          // Sample ID
    bool loaded;
    
    // Non-destructive trim (in samples)
    uint32_t startOffset;
    uint32_t endOffset;
    
    SampleData()
        : data(nullptr)
        , length(0)
        , sampleRate(44100)
        , id(0)
        , loaded(false)
        , startOffset(0)
        , endOffset(0)
    {}
};

/**
 * Sample Part
 * 
 * Manages:
 * - Up to 4 voices with voice stealing
 * - One sample buffer (shared by all voices)
 * - Per-part parameters
 * - Trigger handling from sequencer
 */
class SamplePart {
public:
    SamplePart(uint32_t partIndex);
    ~SamplePart();
    
    // Lifecycle
    void loadSample(const SampleData& sample, bool deferFree = false);
    void unloadSample(bool deferFree = false);
    void releaseRetiredSamples();
    
    // Trigger voice (called from sequencer)
    // Returns false if no sample loaded
    bool trigger(float velocity);
    
    // Render all active voices (called from audio thread)
    // CRITICAL: NO ALLOCATIONS!
    void render(float* leftBuffer, float* rightBuffer, int32_t numFrames);
    
    // Parameter updates (non-blocking, atomic) - forward to voices
    void setPitch(float pitch);
    void setPan(float pan);
    void setLevel(float level);
    void setDecay(float decayMs);
    void setFilter(FilterType filter);
    
    // State queries
    bool hasSample() const { return mSampleLoaded.load(); }
    uint32_t getSampleId() const { return mSampleId.load(); }
    uint32_t getPartIndex() const { return mPartIndex; }
    bool isMuted() const { return mMuted.load(); }
    bool isSoloed() const { return mSoloed.load(); }
    
    // Mute/Solo (M4: UI only, no audio effect yet)
    void setMute(bool muted) { mMuted.store(muted, std::memory_order_relaxed); }
    void setSolo(bool solo) { mSoloed.store(solo, std::memory_order_relaxed); }
    
    // M10: Set sample trim range
    void setTrim(uint32_t startOffset, uint32_t endOffset);
    
    // M6: Set max voices for performance degradation (1-4)
    void setMaxVoices(uint32_t maxVoices);
    
    // Get active voice count (for monitoring)
    uint32_t getActiveVoiceCount() const;

private:
    // Find oldest voice for stealing
    SampleVoice* findOldestVoice();
    
    // Find idle voice
    SampleVoice* findIdleVoice();
    
    // Part index (0-7 for drums, 8 for synth)
    uint32_t mPartIndex;
    
    // Voice pool (fixed size, no allocations)
    SampleVoice mVoices[MAX_VOICES_PER_PART];
    
    // Voice state tracking for stealing
    uint64_t mVoiceStartFrames[MAX_VOICES_PER_PART];  // Frame count when voice started
    
    // Current sample data (owned by part)
    SampleData mSample;
    std::vector<float*> mRetiredSamples;
    std::mutex mRetiredSamplesMutex;
    
    // Sample loaded flag (atomic)
    std::atomic<bool> mSampleLoaded;
    std::atomic<uint32_t> mSampleId;
    std::atomic<uint32_t> mStartOffset;
    std::atomic<uint32_t> mEndOffset;
    
    // Mute/Solo state (M4: UI only)
    std::atomic<bool> mMuted;
    std::atomic<bool> mSoloed;
    
    // Frame counter for voice stealing (int64 to prevent overflow)
    std::atomic<int64_t> mFrameCounter;
    
    // M6: Max voices for performance degradation
    std::atomic<uint32_t> mMaxVoices;
};

} // namespace Tribex

#endif // TRIBEX_SAMPLEPART_H
