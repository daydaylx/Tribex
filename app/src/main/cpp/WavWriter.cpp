#include "WavWriter.h"
#include <cstring>
#include <algorithm>

namespace Tribex {

WavWriter::WavWriter()
    : mSampleRate(44100)
    , mNumChannels(2)
    , mBitsPerSample(16)
    , mBytesPerSample(2)
    , mByteRate(176400)  // 44100 * 2 * 2
    , mDataSizeBytes(0)
    , mNumFramesWritten(0)
{
}

WavWriter::~WavWriter() {
    if (mFile.is_open()) {
        close();
    }
}

bool WavWriter::open(const std::string& filename, 
                   int32_t sampleRate,
                   int32_t numChannels,
                   int32_t bitsPerSample) {
    // Store format parameters
    mSampleRate = sampleRate;
    mNumChannels = numChannels;
    mBitsPerSample = bitsPerSample;
    mBytesPerSample = bitsPerSample / 8;
    mByteRate = sampleRate * numChannels * mBytesPerSample;

    // Reset tracking
    mDataSizeBytes = 0;
    mNumFramesWritten = 0;

    // Open file in binary mode
    mFile.open(filename, std::ios::binary);
    if (!mFile.is_open()) {
        return false;
    }

    // Write initial header (will update sizes at close)
    return writeHeader();
}

bool WavWriter::writeHeader() {
    // RIFF header
    const char riff[4] = {'R', 'I', 'F', 'F'};
    mFile.write(riff, 4);

    // File size (will update at close)
    int32_t fileSize = 36;  // Header size - 8
    mFile.write(reinterpret_cast<const char*>(&fileSize), 4);

    // WAVE format
    const char wave[4] = {'W', 'A', 'V', 'E'};
    mFile.write(wave, 4);

    // fmt chunk
    const char fmt[4] = {'f', 'm', 't', ' '};
    mFile.write(fmt, 4);

    // fmt chunk size (16 for PCM)
    int32_t fmtSize = 16;
    mFile.write(reinterpret_cast<const char*>(&fmtSize), 4);

    // Audio format (1 = PCM)
    int16_t audioFormat = 1;
    mFile.write(reinterpret_cast<const char*>(&audioFormat), 2);

    // Number of channels
    mFile.write(reinterpret_cast<const char*>(&mNumChannels), 2);

    // Sample rate
    mFile.write(reinterpret_cast<const char*>(&mSampleRate), 4);

    // Byte rate
    mFile.write(reinterpret_cast<const char*>(&mByteRate), 4);

    // Block align (channels * bytes per sample)
    int16_t blockAlign = mNumChannels * mBytesPerSample;
    mFile.write(reinterpret_cast<const char*>(&blockAlign), 2);

    // Bits per sample
    mFile.write(reinterpret_cast<const char*>(&mBitsPerSample), 2);

    // data chunk
    const char data[4] = {'d', 'a', 't', 'a'};
    mFile.write(data, 4);

    // Data size (will update at close)
    int32_t dataSize = 0;
    mFile.write(reinterpret_cast<const char*>(&dataSize), 4);

    return mFile.good();
}

bool WavWriter::writeFrames(const float* leftBuffer, 
                          const float* rightBuffer, 
                          int32_t numFrames) {
    if (!mFile.is_open()) {
        return false;
    }

    // Ensure temp buffer is large enough
    size_t numSamples = numFrames * mNumChannels;
    if (mTempBuffer.size() < numSamples) {
        mTempBuffer.resize(numSamples);
    }

    // Convert float32 to int16 (interleaved stereo)
    // First left channel, then right channel
    for (int32_t i = 0; i < numFrames; i++) {
        float leftSample = std::clamp(leftBuffer[i], -1.0f, 1.0f);
        mTempBuffer[i * 2] = static_cast<int16_t>(leftSample * 32767.0f);
        
        if (mNumChannels == 2) {
            float rightSample = std::clamp(rightBuffer[i], -1.0f, 1.0f);
            mTempBuffer[i * 2 + 1] = static_cast<int16_t>(rightSample * 32767.0f);
        }
    }

    // Write to file
    mFile.write(reinterpret_cast<const char*>(mTempBuffer.data()), 
                numSamples * sizeof(int16_t));

    // Update tracking
    mNumFramesWritten += numFrames;
    mDataSizeBytes += numSamples * sizeof(int16_t);

    return mFile.good();
}

void WavWriter::floatToInt16(const float* input, int16_t* output, int32_t numSamples) {
    for (int32_t i = 0; i < numSamples; i++) {
        // Clamp to -1.0 to 1.0, then scale to int16 range
        float sample = std::clamp(input[i], -1.0f, 1.0f);
        output[i] = static_cast<int16_t>(sample * 32767.0f);
    }
}

double WavWriter::getDurationSeconds() const {
    if (mSampleRate == 0) {
        return 0.0;
    }
    return static_cast<double>(mNumFramesWritten) / mSampleRate;
}

bool WavWriter::updateHeaderSizes() {
    // Seek to file size field (offset 4)
    mFile.seekp(4);

    // Total file size = 36 + data size
    int32_t fileSize = 36 + mDataSizeBytes;
    mFile.write(reinterpret_cast<const char*>(&fileSize), 4);

    // Seek to data size field (offset 40)
    mFile.seekp(40);

    // Write data size
    mFile.write(reinterpret_cast<const char*>(&mDataSizeBytes), 4);

    return mFile.good();
}

bool WavWriter::close() {
    if (!mFile.is_open()) {
        return false;
    }

    // Update header with final sizes
    bool success = updateHeaderSizes();

    // Close file
    mFile.close();

    return success;
}

} // namespace Tribex