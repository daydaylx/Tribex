#ifndef TRIBEX_WAVWRITER_H
#define TRIBEX_WAVWRITER_H

#include <cstdint>
#include <vector>
#include <string>
#include <fstream>

namespace Tribex {

/**
 * WAV Writer
 * 
 * Writes audio data to WAV file format (16-bit PCM, stereo).
 * Used for offline export functionality.
 * 
 * Format: 44.1kHz, 16-bit, 2 channels
 * No normalization applied (limiter regulates ceiling)
 */
class WavWriter {
public:
    WavWriter();
    ~WavWriter();

    /**
     * Open WAV file for writing
     * 
     * @param filename Output file path
     * @param sampleRate Sample rate (default 44100 Hz)
     * @param numChannels Number of channels (default 2 for stereo)
     * @param bitsPerSample Bits per sample (default 16)
     * @return true if successful
     */
    bool open(const std::string& filename, 
             int32_t sampleRate = 44100,
             int32_t numChannels = 2,
             int32_t bitsPerSample = 16);

    /**
     * Write audio frames to file
     * 
     * @param leftBuffer Left channel data (float32)
     * @param rightBuffer Right channel data (float32)
     * @param numFrames Number of frames to write
     * @return true if successful
     */
    bool writeFrames(const float* leftBuffer, 
                  const float* rightBuffer, 
                  int32_t numFrames);

    /**
     * Close file and update header with final sizes
     * 
     * @return true if successful
     */
    bool close();

    /**
     * Get current data size in bytes (useful for progress reporting)
     */
    int64_t getDataSizeBytes() const { return mDataSizeBytes; }

    /**
     * Get current duration in seconds
     */
    double getDurationSeconds() const;

private:
    // File handle
    std::ofstream mFile;

    // WAV format parameters
    int32_t mSampleRate;
    int32_t mNumChannels;
    int32_t mBitsPerSample;
    int32_t mBytesPerSample;
    int32_t mByteRate;

    // Tracking
    int64_t mDataSizeBytes;
    int64_t mNumFramesWritten;

    // Temporary buffer for int16 conversion
    std::vector<int16_t> mTempBuffer;

    // Write WAV header
    bool writeHeader();

    // Update header sizes at end of file
    bool updateHeaderSizes();

    // Convert float32 to int16
    void floatToInt16(const float* input, int16_t* output, int32_t numSamples);
};

} // namespace Tribex

#endif // TRIBEX_WAVWRITER_H