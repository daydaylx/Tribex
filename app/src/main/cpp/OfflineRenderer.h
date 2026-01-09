#ifndef TRIBEX_OFFLINERENDERER_H
#define TRIBEX_OFFLINERENDERER_H

#include <cstdint>
#include <atomic>
#include <memory>
#include <functional>
#include <thread>
#include "WavWriter.h"

class AudioEngine;

namespace Tribex {

// Forward declarations
class Pattern;
class Chain;

/**
 * Offline Renderer
 * 
 * Renders audio to WAV file using the same render() logic as live playback.
 * Runs on separate thread, faster than realtime.
 * 
 * Key requirements:
 * - Export must use exact same render() logic as live
 * - Deterministic: same pattern/seed = same output
 * - No allocations in render loop
 * - Progress reporting via callback
 */
class OfflineRenderer {
public:
    using ProgressCallback = std::function<void(double progress, double currentTime)>;

    OfflineRenderer();
    ~OfflineRenderer();

    /**
     * Set the audio engine to use for rendering
     * Must be called before starting export
     */
    void setAudioEngine(::AudioEngine* engine);

    /**
     * Start offline export
     * 
     * @param filename Output WAV file path
     * @param sampleRate Sample rate (default 44100 Hz)
     * @param progressCallback Optional progress callback (0.0 to 1.0)
     * @return true if export started successfully
     */
    bool startExport(const std::string& filename,
                   int32_t sampleRate = 44100,
                   ProgressCallback progressCallback = nullptr);

    /**
     * Check if export is currently running
     */
    bool isExporting() const { return mIsExporting.load(); }

    /**
     * Stop export (thread-safe)
     */
    void stopExport();

    /**
     * Wait for export to complete
     */
    void waitForExportComplete();

    /**
     * Get export result
     * @return true if successful, false if failed
     */
    bool getExportResult() const { return mExportResult.load(); }

    /**
     * Get export error message
     */
    const std::string& getErrorMessage() const { return mErrorMessage; }

    /**
     * Get current export progress (0.0 to 1.0)
     */
    float getProgress() const { return mProgress.load(); }

private:
    // Render thread function
    void renderThreadFunc(const std::string& filename,
                        int32_t sampleRate,
                        ProgressCallback progressCallback);

    // Render audio chunk
    bool renderChunk(float* leftBuffer,
                   float* rightBuffer,
                   int32_t numFrames,
                   int32_t sampleRate);

    // Audio engine reference
    ::AudioEngine* mAudioEngine;

    // Export state
    std::atomic<bool> mIsExporting;
    std::atomic<bool> mStopRequested;
    std::atomic<bool> mExportResult;
    std::atomic<float> mProgress;
    std::string mErrorMessage;

    // Render thread
    std::unique_ptr<std::thread> mRenderThread;

    // Constants
    static constexpr int32_t RENDER_CHUNK_SIZE = 1024;  // Frames per render chunk
};

} // namespace Tribex

#endif // TRIBEX_OFFLINERENDERER_H
