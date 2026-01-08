#include "OfflineRenderer.h"
#include "AudioEngine.h"
#include "Sequencer.h"
#include "PatternData.h"
#include <algorithm>
#include <chrono>

namespace Tribex {

// Helper to get BPM from audio engine
extern double getEngineBPM();  // Will be implemented in AudioEngine

OfflineRenderer::OfflineRenderer()
    : mAudioEngine(nullptr)
    , mIsExporting(false)
    , mStopRequested(false)
    , mExportResult(false)
    , mRenderThread(nullptr)
{
}

OfflineRenderer::~OfflineRenderer() {
    stopExport();
    if (mRenderThread && mRenderThread->joinable()) {
        mRenderThread->join();
    }
}

void OfflineRenderer::setAudioEngine(AudioEngine* engine) {
    mAudioEngine = engine;
}

bool OfflineRenderer::startExport(const std::string& filename,
                                int32_t sampleRate,
                                ProgressCallback progressCallback) {
    if (mIsExporting.load()) {
        mErrorMessage = "Export already in progress";
        return false;
    }

    if (!mAudioEngine) {
        mErrorMessage = "Audio engine not set";
        return false;
    }

    // Reset state
    mStopRequested.store(false);
    mExportResult.store(false);
    mErrorMessage.clear();
    mIsExporting.store(true);

    // Start render thread
    mRenderThread = std::make_unique<std::thread>(
        &OfflineRenderer::renderThreadFunc,
        this,
        filename,
        sampleRate,
        progressCallback
    );

    return true;
}

void OfflineRenderer::stopExport() {
    mStopRequested.store(true);
}

void OfflineRenderer::waitForExportComplete() {
    if (mRenderThread && mRenderThread->joinable()) {
        mRenderThread->join();
    }
}

void OfflineRenderer::renderThreadFunc(const std::string& filename,
                                   int32_t sampleRate,
                                   ProgressCallback progressCallback) {
    // Open WAV writer
    Tribex::WavWriter wavWriter;
    if (!wavWriter.open(filename, sampleRate, 2, 16)) {
        mErrorMessage = "Failed to open WAV file: " + filename;
        mExportResult.store(false);
        mIsExporting.store(false);
        return;
    }

    // Preallocate buffers (NO allocations in render loop!)
    float leftBuffer[RENDER_CHUNK_SIZE];
    float rightBuffer[RENDER_CHUNK_SIZE];

    // Get total frames to render (for progress)
    // Note: For now, we'll calculate progress based on chain iteration
    // Full chain duration calculation will be in AudioEngine integration
    
    double totalDurationSeconds = 0.0;
    int32_t totalFrames = 0;

    // Render loop
    bool rendering = true;
    int64_t sampleCounter = 0;  // Offline sample counter
    
    while (rendering && !mStopRequested.load()) {
        // Render audio chunk
        bool success = renderChunk(leftBuffer, rightBuffer, 
                                 RENDER_CHUNK_SIZE, 
                                 sampleRate);
        
        if (!success) {
            mErrorMessage = "Render failed during export";
            rendering = false;
            break;
        }

        // Check if export is complete
        // This will be updated when AudioEngine integration is done
        // For now, we'll stop after a fixed duration (placeholder)
        
        // Write to WAV file
        wavWriter.writeFrames(leftBuffer, rightBuffer, RENDER_CHUNK_SIZE);
        
        sampleCounter += RENDER_CHUNK_SIZE;

        // Update progress
        if (progressCallback) {
            double currentTime = static_cast<double>(sampleCounter) / sampleRate;
            // Progress will be 0.0 to 1.0 when chain duration is known
            progressCallback(0.5, currentTime);  // Placeholder
        }

        // For M7: We'll break when chain is complete
        // This requires AudioEngine to expose chain state
        // For now, render 1 minute as placeholder
        if (sampleCounter >= sampleRate * 60) {
            rendering = false;
        }
    }

    // Close WAV file
    if (wavWriter.close()) {
        mExportResult.store(true);
    } else {
        mErrorMessage = "Failed to close WAV file";
        mExportResult.store(false);
    }

    mIsExporting.store(false);
}

bool OfflineRenderer::renderChunk(float* leftBuffer,
                               float* rightBuffer,
                               int32_t numFrames,
                               int32_t sampleRate) {
    if (!mAudioEngine) {
        return false;
    }

    // Clear buffers
    for (int32_t i = 0; i < numFrames; i++) {
        leftBuffer[i] = 0.0f;
        rightBuffer[i] = 0.0f;
    }

    // Note: This is a placeholder implementation
    // The actual implementation will call AudioEngine::renderOffline()
    // which contains the exact same logic as the live callback
    //
    // The AudioEngine integration will provide:
    // 1. Event processing
    // 2. Sequencer step evaluation
    // 3. Part rendering (drums + synth)
    // 4. FX chain processing
    // 5. Pattern chain iteration
    
    // For now, generate silence (will be replaced)
    return true;
}

} // namespace Tribex