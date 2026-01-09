package com.tribex.groovebox.engine

import android.util.Log

/**
 * AudioEngineBridge - JNI Interface Wrapper
 * 
 * Provides a clean Kotlin interface to the native audio engine.
 * All methods are non-blocking - events are pushed to a lock-free queue.
 * 
 * Thread Safety:
 * - Call from UI Thread (Control Thread) only
 * - Never call from Audio Thread
 */
object AudioEngineBridge {
    private const val TAG = "AudioEngineBridge"
    
    // Load native library
    init {
        System.loadLibrary("tribex_native")
    }
    
    /**
     * Start the audio engine
     */
    external fun startAudioEngine(): Boolean
    
    /**
     * Stop the audio engine
     */
    external fun stopAudioEngine(): Boolean
    
    /**
     * Check if audio is currently playing
     */
    external fun isPlaying(): Boolean
    
    /**
     * Toggle audio start/stop
     */
    external fun toggleNativeAudio()
    
    /**
     * Cleanup - stop and destroy engine
     */
    external fun cleanup()
    
    /**
     * Set master gain (0.0 to 2.0)
     * 
     * Non-blocking - pushes event to queue
     * Values outside range are clamped in native code
     */
    external fun setMasterGain(gain: Float)
    
    /**
     * Set master pan (-1.0 to 1.0)
     * 
     * Non-blocking - pushes event to queue
     * -1.0 = Left, 0.0 = Center, 1.0 = Right
     */
    external fun setMasterPan(pan: Float)
    
    /**
     * Set test tone frequency (20Hz to 20kHz)
     * 
     * Non-blocking - pushes event to queue
     * Values outside range are clamped in native code
     */
    external fun setTestToneFrequency(freq: Float)
    
    /**
     * Clear all pending events
     * 
     * Call this when resetting UI state
     */
    external fun clearEvents()
    
    // M2 NEU: Sequencer Control Methods
    
    /**
     * Set BPM (20.0 to 300.0)
     * 
     * Non-blocking - pushes event to queue
     * Values outside range are clamped in native code
     */
    external fun setBPM(bpm: Float)
    
    /**
     * Start the sequencer
     */
    external fun startSequencer()
    
    /**
     * Stop the sequencer
     */
    external fun stopSequencer()
    
    /**
     * Check if sequencer is currently playing
     */
    external fun isSequencerPlaying(): Boolean
    
    /**
     * P0.4: Get current step index (0-63)
     * 
     * Thread-safe - reads from atomic in Sequencer
     */
    external fun getCurrentStep(): Int
    
    /**
     * P0.4: Get current loop iteration
     * 
     * Thread-safe - reads from atomic in Sequencer
     */
    external fun getLoopIteration(): Int
    
    /**
     * Set pattern in sequencer
     * 
     * @param patternData Pattern data as ByteArray (serialized C++ Pattern structure)
     * @param patternLength Pattern length in steps (16, 32, 48, or 64)
     * @param patternSeed Pattern seed for deterministic probability
     * 
     * Note: This is a direct call (not via event queue) since patterns are too large
     */
    external fun setPattern(patternData: ByteArray, patternLength: Int, patternSeed: Int)
    
    // M4 NEU: Sample Engine Control Methods
    
    /**
     * Load sample into a part
     * 
     * @param partIndex Part index (0-7 for drums, 8 for synth)
     * @param sampleData Sample data as byte array (float32)
     * @param length Length of sample data in bytes
     * @param sampleRate Original sample rate of the sample
     * @param sampleId Unique sample ID
     * @param startOffset Start offset in samples (for trimming)
     * @param endOffset End offset in samples (for trimming, 0 = end of sample)
     * 
     * Note: This is called from IO thread, not blocking
     */
    external fun loadSample(
        partIndex: Int,
        sampleData: ByteArray,
        length: Int,
        sampleRate: Int,
        sampleId: Int,
        startOffset: Int,
        endOffset: Int
    )
    
    /**
     * Unload sample from a part
     * 
     * @param partIndex Part index (0-7 for drums, 8 for synth)
     */
    external fun unloadSample(partIndex: Int)
    
    /**
     * Set voice pitch for a part
     * 
     * @param partIndex Part index (0-7 for drums, 8 for synth)
     * @param pitch Pitch in semitones (-24 to +24)
     * 
     * Non-blocking - pushes event to queue
     */
    external fun setVoicePitch(partIndex: Int, pitch: Float)
    
    /**
     * Set voice pan for a part
     * 
     * @param partIndex Part index (0-7 for drums, 8 for synth)
     * @param pan Pan (-1.0 to 1.0, -1.0 = Left, 0.0 = Center, 1.0 = Right)
     * 
     * Non-blocking - pushes event to queue
     */
    external fun setVoicePan(partIndex: Int, pan: Float)
    
    /**
     * Set voice level for a part
     * 
     * @param partIndex Part index (0-7 for drums, 8 for synth)
     * @param level Level (0.0 to 1.0)
     * 
     * Non-blocking - pushes event to queue
     */
    external fun setVoiceLevel(partIndex: Int, level: Float)
    
    /**
     * Set voice decay for a part
     * 
     * @param partIndex Part index (0-7 for drums, 8 for synth)
     * @param decayMs Decay in milliseconds (0.0 to 5000.0)
     * 
     * Non-blocking - pushes event to queue
     */
    external fun setVoiceDecay(partIndex: Int, decayMs: Float)
    
    /**
     * Set voice filter type for a part
     * 
     * @param partIndex Part index (0-7 for drums, 8 for synth)
     * @param filterType Filter type (0 = Low Pass, 1 = High Pass)
     * 
     * Non-blocking - pushes event to queue
     */
    external fun setVoiceFilter(partIndex: Int, filterType: Int)
    
    /**
     * Set voice trim range
     * 
     * @param partIndex Part index (0-7 for drums)
     * @param startOffset Start offset in samples
     * @param endOffset End offset in samples (0 = end of sample)
     * 
     * Non-blocking - updates atomic parameters
     */
    external fun setVoiceTrim(partIndex: Int, startOffset: Int, endOffset: Int)
    
    /**
     * Mute a part
     * 
     * @param partIndex Part index (0-7 for drums, 8 for synth)
     * @param muted true to mute, false to unmute
     * 
     * Note: M4: UI only, no audio effect yet
     */
    external fun setPartMute(partIndex: Int, muted: Boolean)
    
    /**
     * Solo a part
     * 
     * @param partIndex Part index (0-7 for drums, 8 for synth)
     * @param solo true to solo, false to un-solo
     * 
     * Note: M4: UI only, no audio effect yet
     */
    external fun setPartSolo(partIndex: Int, solo: Boolean)
    
    // M5 NEU: Synth Part Control Methods (Part 8 only)
    
    /**
     * Set synth wavetable type
     * 
     * @param partIndex Part index (must be 8 for synth)
     * @param wavetableType Wavetable type (0=Saw, 1=Square, 2=Sine, 3=Major, 4=Minor, 5=7th)
     * 
     * Non-blocking - pushes event to queue
     */
    external fun setSynthWavetable(partIndex: Int, wavetableType: Int)
    
    /**
     * Set synth filter cutoff
     * 
     * @param partIndex Part index (must be 8 for synth)
     * @param cutoff Normalized cutoff (0.0 to 1.0)
     * 
     * Non-blocking - pushes event to queue
     */
    external fun setSynthCutoff(partIndex: Int, cutoff: Float)
    
    /**
     * Set synth filter resonance
     * 
     * @param partIndex Part index (must be 8 for synth)
     * @param resonance Resonance (0.0 to 1.0)
     * 
     * Non-blocking - pushes event to queue
     */
    external fun setSynthResonance(partIndex: Int, resonance: Float)
    
    /**
     * Set synth ADSR attack
     * 
     * @param partIndex Part index (must be 8 for synth)
     * @param attackMs Attack time in milliseconds (0.0 to 5000.0)
     * 
     * Non-blocking - pushes event to queue
     */
    external fun setSynthAttack(partIndex: Int, attackMs: Float)
    
    /**
     * Set synth ADSR decay
     * 
     * @param partIndex Part index (must be 8 for synth)
     * @param decayMs Decay time in milliseconds (0.0 to 5000.0)
     * 
     * Non-blocking - pushes event to queue
     */
    external fun setSynthDecay(partIndex: Int, decayMs: Float)
    
    /**
     * Set synth ADSR sustain
     * 
     * @param partIndex Part index (must be 8 for synth)
     * @param sustainLevel Sustain level (0.0 to 1.0)
     * 
     * Non-blocking - pushes event to queue
     */
    external fun setSynthSustain(partIndex: Int, sustainLevel: Float)
    
    /**
     * Set synth ADSR release
     * 
     * @param partIndex Part index (must be 8 for synth)
     * @param releaseMs Release time in milliseconds (0.0 to 5000.0)
     * 
     * Non-blocking - pushes event to queue
     */
    external fun setSynthRelease(partIndex: Int, releaseMs: Float)
    
    // Helper methods with validation
    
    /**
     * Set master gain with automatic clamping
     */
    fun setMasterGainClamped(gain: Float) {
        val clampedGain = gain.coerceIn(0.0f, 2.0f)
        setMasterGain(clampedGain)
        Log.d(TAG, "Master gain set to: $clampedGain")
    }
    
    /**
     * Set master pan with automatic clamping
     */
    fun setMasterPanClamped(pan: Float) {
        val clampedPan = pan.coerceIn(-1.0f, 1.0f)
        setMasterPan(clampedPan)
        Log.d(TAG, "Master pan set to: $clampedPan")
    }
    
    /**
     * Set test tone frequency with automatic clamping
     */
    fun setTestToneFrequencyClamped(freq: Float) {
        val clampedFreq = freq.coerceIn(20.0f, 20000.0f)
        setTestToneFrequency(clampedFreq)
        Log.d(TAG, "Test tone frequency set to: $clampedFreq Hz")
    }
    
    // M6 NEU: FX Control Methods
    
    /**
     * Set delay time
     * 
     * @param timeMs Delay time in milliseconds (0.0 to 1000.0)
     * Non-blocking - updates atomic parameter
     */
    external fun setDelayTimeMs(timeMs: Float)
    
    /**
     * Set delay feedback
     * 
     * @param feedback Feedback amount (0.0 to 0.95)
     * Non-blocking - updates atomic parameter
     */
    external fun setDelayFeedback(feedback: Float)
    
    /**
     * Set delay mix (wet/dry balance)
     * 
     * @param mix Mix amount (0.0 = dry, 1.0 = wet)
     * Non-blocking - updates atomic parameter
     */
    external fun setDelayMix(mix: Float)
    
    /**
     * Set reverb size
     * 
     * @param size Reverb size (0.0 = small, 1.0 = large)
     * Non-blocking - updates atomic parameter
     */
    external fun setReverbSize(size: Float)
    
    /**
     * Set reverb density
     * 
     * @param density Reverb density (0.0 = sparse, 1.0 = dense)
     * Non-blocking - updates atomic parameter
     */
    external fun setReverbDensity(density: Float)
    
    /**
     * Set reverb mix (wet/dry balance)
     * 
     * @param mix Mix amount (0.0 = dry, 1.0 = wet)
     * Non-blocking - updates atomic parameter
     */
    external fun setReverbMix(mix: Float)
    
    /**
     * Set valve saturation amount
     * 
     * @param amount Saturation amount (0.0 = clean, 1.0 = saturated)
     * Non-blocking - updates atomic parameter
     */
    external fun setValveAmount(amount: Float)
    
    /**
     * Set limiter threshold
     * 
     * @param thresholdDb Threshold in dB (-12.0 to -0.3)
     * Non-blocking - updates atomic parameter
     */
    external fun setLimiterThresholdDb(thresholdDb: Float)
    
    /**
     * Set limiter release time
     * 
     * @param releaseMs Release time in milliseconds (10 to 1000)
     * Non-blocking - updates atomic parameter
     */
    external fun setLimiterReleaseMs(releaseMs: Float)
    
    // M6 NEU: Degradation Control Methods
    
    /**
     * Set performance degradation level
     * 
     * @param level Degradation level (0 = optimal, 1 = warning, 2 = critical)
     * Level 0: Max 24 voices, reverb high, valve active
     * Level 1: Max 16 voices, reverb low, valve active
     * Level 2: Max 8 voices, reverb off, valve bypass
     */
    external fun setDegradationLevel(level: Int)
    
    /**
     * Get current performance degradation level
     * 
     * @return Degradation level (0 = optimal, 1 = warning, 2 = critical)
     */
    external fun getDegradationLevel(): Int
    
    /**
     * Get maximum voices for current degradation level
     * 
     * @return Maximum voices (24, 16, or 8 depending on degradation level)
     */
    external fun getMaxVoices(): Int
    
    /**
     * Reset XRun counter
     * 
     * Call this periodically to try to downgrade degradation level
     * when audio is stable
     */
    external fun resetXRunCounter()
    
    // P1.2: Export Methods
    
    /**
     * Start offline export
     * 
     * @param filename Output WAV file path
     * @return true if export started successfully
     */
    external fun startExport(filename: String): Boolean
    
    /**
     * Stop export (thread-safe)
     */
    external fun stopExport()
    
    /**
     * Get export progress (0.0 to 1.0)
     * 
     * @return Progress value (0.0 = not started, 1.0 = complete)
     */
    external fun getExportProgress(): Float
    
    /**
     * Check if export is currently running
     * 
     * @return true if export is in progress
     */
    external fun isExporting(): Boolean
}