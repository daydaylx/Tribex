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
}