#include "SampleLoader.h"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace Tribex {

SampleLoaderResult SampleLoader::loadWAV(const std::string& filePath) {
    SampleLoaderResult result;
    
    // Open file
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        result.error = "Failed to open file: " + filePath;
        return result;
    }
    
    // Read RIFF header
    RiffChunk riff;
    file.read(reinterpret_cast<char*>(&riff), sizeof(RiffChunk));
    
    // Read fmt chunk
    FmtChunk fmt;
    file.read(reinterpret_cast<char*>(&fmt), sizeof(FmtChunk));
    
    // Validate headers
    if (!validateHeaders(riff, fmt)) {
        result.error = "Invalid WAV file format";
        return result;
    }
    
    // Check audio format (only PCM supported)
    if (fmt.audioFormat != 1) {
        result.error = "Only PCM format is supported";
        return result;
    }
    
    // Check bits per sample (16, 24, or 32)
    if (fmt.bitsPerSample != 16 && fmt.bitsPerSample != 24 && fmt.bitsPerSample != 32) {
        result.error = "Only 16, 24, or 32-bit PCM is supported";
        return result;
    }
    
    // Check sample rate (44.1kHz or 48kHz recommended)
    if (fmt.sampleRate != 44100 && fmt.sampleRate != 48000) {
        // Warning, but allow
        result.error = "Warning: Sample rate " + std::to_string(fmt.sampleRate) + " may require resampling";
        // Don't return error, just continue
    }
    
    // Find data chunk
    DataChunk dataChunk;
    bool foundData = false;
    
    while (!file.eof() && !foundData) {
        char chunkID[4];
        uint32_t chunkSize;
        
        file.read(chunkID, 4);
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        
        if (std::strncmp(chunkID, "data", 4) == 0) {
            foundData = true;
            dataChunk.subchunk2Size = chunkSize;
        } else {
            // Skip unknown chunk
            file.seekg(chunkSize, std::ios::cur);
        }
    }
    
    if (!foundData) {
        result.error = "No data chunk found in WAV file";
        return result;
    }
    
    // Calculate number of samples
    uint32_t bytesPerSample = fmt.bitsPerSample / 8;
    uint32_t numSamples = dataChunk.subchunk2Size / (fmt.numChannels * bytesPerSample);
    
    // Read audio data
    uint8_t* audioData = new (std::nothrow) uint8_t[dataChunk.subchunk2Size];
    if (audioData == nullptr) {
        result.error = "Failed to allocate memory for audio data";
        return result;
    }
    
    file.read(reinterpret_cast<char*>(audioData), dataChunk.subchunk2Size);
    file.close();
    
    // Convert to float based on bit depth
    float* floatData = nullptr;
    
    if (fmt.bitsPerSample == 16) {
        floatData = readPCM16(audioData, numSamples, fmt.numChannels);
    } else if (fmt.bitsPerSample == 24) {
        floatData = readPCM24(audioData, numSamples, fmt.numChannels);
    } else if (fmt.bitsPerSample == 32) {
        floatData = readPCM32(audioData, numSamples, fmt.numChannels);
    }
    
    delete[] audioData;
    
    if (floatData == nullptr) {
        result.error = "Failed to convert audio data to float";
        return result;
    }
    
    // Convert stereo to mono if needed
    float* finalData = nullptr;
    if (fmt.numChannels == 2) {
        finalData = convertToMono(floatData, numSamples);
        delete[] floatData;
    } else {
        finalData = floatData;
    }
    
    if (finalData == nullptr) {
        result.error = "Failed to convert to mono";
        return result;
    }
    
    // Return result
    result.data = finalData;
    result.length = (fmt.numChannels == 2) ? (numSamples / 2) : numSamples;
    result.sampleRate = fmt.sampleRate;
    result.success = true;
    
    return result;
}

SampleLoaderResult SampleLoader::loadWAVFromMemory(const uint8_t* buffer, uint32_t bufferSize) {
    SampleLoaderResult result;
    
    if (buffer == nullptr || bufferSize < sizeof(RiffChunk) + sizeof(FmtChunk)) {
        result.error = "Invalid buffer";
        return result;
    }
    
    uint32_t offset = 0;
    
    // Read RIFF header
    RiffChunk riff;
    std::memcpy(&riff, buffer + offset, sizeof(RiffChunk));
    offset += sizeof(RiffChunk);
    
    // Read fmt chunk
    FmtChunk fmt;
    std::memcpy(&fmt, buffer + offset, sizeof(FmtChunk));
    offset += sizeof(FmtChunk);
    
    // Validate headers
    if (!validateHeaders(riff, fmt)) {
        result.error = "Invalid WAV file format";
        return result;
    }
    
    // Check audio format (only PCM supported)
    if (fmt.audioFormat != 1) {
        result.error = "Only PCM format is supported";
        return result;
    }
    
    // Check bits per sample (16, 24, or 32)
    if (fmt.bitsPerSample != 16 && fmt.bitsPerSample != 24 && fmt.bitsPerSample != 32) {
        result.error = "Only 16, 24, or 32-bit PCM is supported";
        return result;
    }
    
    // Find data chunk
    DataChunk dataChunk;
    bool foundData = false;
    
    while (offset < bufferSize && !foundData) {
        char chunkID[4];
        uint32_t chunkSize;
        
        if (offset + 8 > bufferSize) break;
        
        std::memcpy(chunkID, buffer + offset, 4);
        offset += 4;
        std::memcpy(&chunkSize, buffer + offset, 4);
        offset += 4;
        
        if (std::strncmp(chunkID, "data", 4) == 0) {
            foundData = true;
            dataChunk.subchunk2Size = chunkSize;
        } else {
            // Skip unknown chunk
            offset += chunkSize;
        }
    }
    
    if (!foundData || offset + dataChunk.subchunk2Size > bufferSize) {
        result.error = "No data chunk found or invalid buffer size";
        return result;
    }
    
    // Calculate number of samples
    uint32_t bytesPerSample = fmt.bitsPerSample / 8;
    uint32_t numSamples = dataChunk.subchunk2Size / (fmt.numChannels * bytesPerSample);
    
    // Pointer to audio data
    const uint8_t* audioData = buffer + offset;
    
    // Convert to float based on bit depth
    float* floatData = nullptr;
    
    if (fmt.bitsPerSample == 16) {
        floatData = readPCM16(audioData, numSamples, fmt.numChannels);
    } else if (fmt.bitsPerSample == 24) {
        floatData = readPCM24(audioData, numSamples, fmt.numChannels);
    } else if (fmt.bitsPerSample == 32) {
        floatData = readPCM32(audioData, numSamples, fmt.numChannels);
    }
    
    if (floatData == nullptr) {
        result.error = "Failed to convert audio data to float";
        return result;
    }
    
    // Convert stereo to mono if needed
    float* finalData = nullptr;
    if (fmt.numChannels == 2) {
        finalData = convertToMono(floatData, numSamples);
        delete[] floatData;
    } else {
        finalData = floatData;
    }
    
    if (finalData == nullptr) {
        result.error = "Failed to convert to mono";
        return result;
    }
    
    // Return result
    result.data = finalData;
    result.length = (fmt.numChannels == 2) ? (numSamples / 2) : numSamples;
    result.sampleRate = fmt.sampleRate;
    result.success = true;
    
    return result;
}

float* SampleLoader::convertToMono(const float* data, uint32_t length) {
    if (data == nullptr) {
        return nullptr;
    }
    
    // Allocate mono buffer (half the size of stereo)
    uint32_t monoLength = length / 2;
    float* monoData = new (std::nothrow) float[monoLength];
    
    if (monoData == nullptr) {
        return nullptr;
    }
    
    // Average left and right channels
    for (uint32_t i = 0; i < monoLength; i++) {
        float left = data[i * 2];
        float right = data[i * 2 + 1];
        monoData[i] = (left + right) * 0.5f;
    }
    
    return monoData;
}

bool SampleLoader::validateHeaders(const RiffChunk& riff, const FmtChunk& fmt) {
    // Check RIFF header
    if (std::strncmp(riff.chunkID, "RIFF", 4) != 0) {
        return false;
    }
    
    // Check WAVE format
    if (std::strncmp(riff.format, "WAVE", 4) != 0) {
        return false;
    }
    
    // Check fmt chunk ID
    if (std::strncmp(fmt.subchunk1ID, "fmt ", 4) != 0) {
        return false;
    }
    
    return true;
}

float* SampleLoader::readPCM16(const uint8_t* data, uint32_t length, int numChannels) {
    float* floatData = new (std::nothrow) float[length];
    
    if (floatData == nullptr) {
        return nullptr;
    }
    
    // Convert 16-bit PCM to float (-1.0 to 1.0)
    for (uint32_t i = 0; i < length; i++) {
        int16_t sample;
        std::memcpy(&sample, data + i * 2 * numChannels, 2);
        floatData[i] = static_cast<float>(sample) / 32768.0f;
    }
    
    return floatData;
}

float* SampleLoader::readPCM24(const uint8_t* data, uint32_t length, int numChannels) {
    float* floatData = new (std::nothrow) float[length];
    
    if (floatData == nullptr) {
        return nullptr;
    }
    
    // Convert 24-bit PCM to float (-1.0 to 1.0)
    for (uint32_t i = 0; i < length; i++) {
        int32_t sample = 0;
        const uint8_t* bytePtr = data + i * 3 * numChannels;
        
        // Little-endian 24-bit
        sample = bytePtr[0] | (bytePtr[1] << 8) | (bytePtr[2] << 16);
        
        // Sign-extend from 24 to 32 bits
        if (sample & 0x800000) {
            sample |= 0xFF000000;
        }
        
        floatData[i] = static_cast<float>(sample) / 8388608.0f;  // 2^23
    }
    
    return floatData;
}

float* SampleLoader::readPCM32(const uint8_t* data, uint32_t length, int numChannels) {
    float* floatData = new (std::nothrow) float[length];
    
    if (floatData == nullptr) {
        return nullptr;
    }
    
    // Convert 32-bit PCM to float (-1.0 to 1.0)
    for (uint32_t i = 0; i < length; i++) {
        int32_t sample;
        std::memcpy(&sample, data + i * 4 * numChannels, 4);
        floatData[i] = static_cast<float>(sample) / 2147483648.0f;  // 2^31
    }
    
    return floatData;
}

} // namespace Tribex