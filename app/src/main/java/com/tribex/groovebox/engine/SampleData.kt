package com.tribex.groovebox.engine

/**
 * Sample Data Models
 * 
 * Data classes for sample metadata and parameters.
 * Used for communication between UI and Audio Engine.
 */

/**
 * Filter Type for Voice
 */
enum class FilterType(val id: Int) {
    LOW_PASS(0),
    HIGH_PASS(1)
}

/**
 * Voice Parameters
 * 
 * Parameters that can be adjusted per voice:
 * - Pitch: Semitones (-24 to +24)
 * - Pan: Stereo position (-1.0 to 1.0)
 * - Level: Volume (0.0 to 1.0)
 * - Decay: Envelope decay in ms (0.0 to 5000.0)
 * - Filter: LP or HP
 */
data class VoiceParams(
    var pitch: Float = 0.0f,
    var pan: Float = 0.0f,
    var level: Float = 1.0f,
    var decayMs: Float = 200.0f,
    var filter: FilterType = FilterType.LOW_PASS
) {
    companion object {
        val DEFAULT = VoiceParams()
        
        val PITCH_RANGE = -24.0f..24.0f
        val PAN_RANGE = -1.0f..1.0f
        val LEVEL_RANGE = 0.0f..1.0f
        val DECAY_RANGE = 0.0f..5000.0f
    }
    
    /**
     * Clamp all parameters to valid ranges
     */
    fun clamp(): VoiceParams {
        return copy(
            pitch = pitch.coerceIn(PITCH_RANGE),
            pan = pan.coerceIn(PAN_RANGE),
            level = level.coerceIn(LEVEL_RANGE),
            decayMs = decayMs.coerceIn(DECAY_RANGE)
        )
    }
}

/**
 * Sample Metadata
 * 
 * Information about a loaded sample:
 * - id: Unique identifier
 * - name: Display name
 * - lengthMs: Duration in milliseconds
 * - sampleRate: Original sample rate
 * - startOffset: Trim start offset in samples
 * - endOffset: Trim end offset in samples (0 = end of sample)
 */
data class SampleMetadata(
    val id: Int = 0,
    val name: String = "",
    val lengthMs: Long = 0L,
    val sampleRate: Int = 44100,
    var startOffset: Int = 0,
    var endOffset: Int = 0
) {
    /**
     * Get trim range in percentage (0.0 to 1.0)
     */
    fun getTrimRange(): Pair<Float, Float> {
        val totalSamples = (lengthMs * sampleRate) / 1000
        val startPercent = if (totalSamples > 0) startOffset.toFloat() / totalSamples else 0.0f
        val endPercent = if (totalSamples > 0 && endOffset > 0) endOffset.toFloat() / totalSamples else 1.0f
        return startPercent.coerceIn(0.0f, 1.0f) to endPercent.coerceIn(0.0f, 1.0f)
    }
    
    /**
     * Set trim range from percentage (0.0 to 1.0)
     */
    fun setTrimRange(startPercent: Float, endPercent: Float) {
        val totalSamples = (lengthMs * sampleRate) / 1000
        if (totalSamples > 0) {
            startOffset = (startPercent.coerceIn(0.0f, 1.0f) * totalSamples).toInt()
            endOffset = if (endPercent >= 1.0f) 0 else (endPercent.coerceIn(0.0f, 1.0f) * totalSamples).toInt()
        }
    }
}

/**
 * Part Sample State
 * 
 * State of a part's sample assignment:
 * - partIndex: Part index (0-7 for drums, 8 for synth)
 * - sampleId: Currently assigned sample ID
 * - metadata: Sample metadata
 * - params: Voice parameters
 * - muted: Part muted state
 * - soloed: Part soloed state
 * - hasSample: Whether a sample is loaded
 */
data class PartSampleState(
    val partIndex: Int = 0,
    val sampleId: Int = 0,
    val metadata: SampleMetadata? = null,
    val params: VoiceParams = VoiceParams.DEFAULT,
    val muted: Boolean = false,
    val soloed: Boolean = false,
    val hasSample: Boolean = false
) {
    companion object {
        /**
         * Create empty state for all 9 parts
         */
        fun createEmpty(): List<PartSampleState> {
            return (0 until 9).map { PartSampleState(partIndex = it) }
        }
    }
}

/**
 * Sample Import Result
 * 
 * Result of sample loading operation:
 * - success: Whether loading succeeded
 * - sampleId: Unique sample ID if successful
 * - metadata: Sample metadata if successful
 * - sampleData: Float32 audio data if successful
 * - error: Error message if failed
 */
data class SampleImportResult(
    val success: Boolean = false,
    val sampleId: Int = 0,
    val metadata: SampleMetadata? = null,
    val sampleData: ByteArray? = null,
    val error: String? = null
)