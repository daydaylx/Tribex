#ifndef TRIBEX_SAMPLELOADER_H
#define TRIBEX_SAMPLELOADER_H

#include "SamplePart.h"
#include <cstdint>
#include <string>

namespace Tribex {

/**
 * Sample Loader - WAV File Parser
 * 
 * Loads WAV files and converts to float audio data.
 * This is called from IO Thread (NOT audio thread) - allocations are allowed.
 * 
 * Supported formats:
 * - 16-bit PCM
 * - 24-bit PCM
 * - 32-bit PCM
 * - Sample rates: 44.1kHz, 48kHz
 * - Mono or Stereo (converted to mono)
 */

// WAV file headers
#pragma pack(push, 1)
struct RiffChunk {
    char chunkID[4];      // "RIFF"
    uint32_t chunkSize;   // File size - 8
    char format[4];       // "WAVE"
};

struct FmtChunk {
    char subchunk1ID[4];   // "fmt "
    uint32_t subchunk1Size; // 16 for PCM
    uint16_t audioFormat;    // 1 = PCM
    uint16_t numChannels;   // 1 = mono, 2 = stereo
    uint32_t sampleRate;    // 44100, 48000, etc.
    uint32_t byteRate;      // sampleRate * numChannels * bitsPerSample/8
    uint16_t blockAlign;     // numChannels * bitsPerSample/8
    uint16_t bitsPerSample;  // 8, 16, 24, 32
};

struct DataChunk {
    char subchunk2ID[4];   // "data"
    uint32_t subchunk2Size; // NumSamples * numChannels * bitsPerSample/8
};
#pragma pack(pop)

/**
 * Sample Loader Result
 * Contains loaded sample data and metadata
 */
struct SampleLoaderResult {
    float* data;           // Audio data (float, mono)
    uint32_t length;       // Length in samples
    uint32_t sampleRate;   // Original sample rate
    bool success;
    std::string error;      // Error message if failed
    
    SampleLoaderResult()
        : data(nullptr)
        , length(0)
        , sampleRate(0)
        , success(false)
        , error("")
    {}
};

/**
 * Sample Loader
 * 
 * Provides static methods for loading WAV files.
 * All operations are synchronous and may allocate memory.
 */
class SampleLoader {
public:
    /**
     * Load WAV file from path
     * 
     * @param filePath Path to WAV file
     * @return SampleLoaderResult with loaded data
     * 
     * NOTE: Caller is responsible for freeing result.data with delete[]
     */
    static SampleLoaderResult loadWAV(const std::string& filePath);
    
    /**
     * Load WAV file from memory buffer
     * 
     * @param buffer Pointer to WAV file data in memory
     * @param bufferSize Size of buffer in bytes
     * @return SampleLoaderResult with loaded data
     * 
     * NOTE: Caller is responsible for freeing result.data with delete[]
     */
    static SampleLoaderResult loadWAVFromMemory(const uint8_t* buffer, uint32_t bufferSize);
    
    /**
     * Convert sample data to mono
     * 
     * @param data Input stereo data (interleaved)
     * @param length Length in frames (not samples)
     * @return Float array with mono data (caller must delete[])
     */
    static float* convertToMono(const float* data, uint32_t length);
    
private:
    /**
     * Validate WAV file headers
     */
    static bool validateHeaders(const RiffChunk& riff, const FmtChunk& fmt);
    
    /**
     * Read 16-bit PCM data
     */
    static float* readPCM16(const uint8_t* data, uint32_t length, int numChannels);
    
    /**
     * Read 24-bit PCM data
     */
    static float* readPCM24(const uint8_t* data, uint32_t length, int numChannels);
    
    /**
     * Read 32-bit PCM data
     */
    static float* readPCM32(const uint8_t* data, uint32_t length, int numChannels);
};

} // namespace Tribex

#endif // TRIBEX_SAMPLELOADER_H