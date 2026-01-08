package com.tribex.groovebox.persistence.entities

import androidx.room.Entity
import androidx.room.ForeignKey
import androidx.room.PrimaryKey

/**
 * Sample Entity - Stores sample metadata and file references
 * 
 * Part index: 0-7 for drum parts, 8 for synth part
 * Actual WAV files stored in /samples/ directory
 * Waveform data stored as blob for quick visualization
 */
@Entity(
    tableName = "samples",
    foreignKeys = [
        ForeignKey(
            entity = Project::class,
            parentColumns = ["id"],
            childColumns = ["projectId"],
            onDelete = ForeignKey.CASCADE
        )
    ],
    indices = [
        androidx.room.Index(value = ["projectId"]),
        androidx.room.Index(value = ["projectId", "partIndex"], unique = true)
    ]
)
data class Sample(
    @PrimaryKey
    val id: String,
    
    val projectId: String,
    
    /**
     * Part index (0-7 for drum parts, 8 for synth)
     */
    val partIndex: Int,  // 0-7: Drum parts, 8: Synth
    
    /**
     * WAV filename in /samples/ directory
     */
    val filename: String,  // e.g., "sample_001.wav"
    
    /**
     * Original import path (for reference only)
     */
    val originalPath: String? = null,
    
    /**
     * Voice parameters
     */
    val pitch: Float = 0.0f,  // -24 to +24 semitones
    val pan: Float = 0.0f,  // -1.0 (L) to 1.0 (R)
    val level: Float = 0.8f,  // 0.0 to 1.0
    val decay: Float = 200.0f,  // ms, 0 to 5000
    val filterType: Int = 0,  // 0: LPF, 1: HPF
    
    /**
     * Trim (non-destructive)
     */
    val trimStart: Int = 0,  // Offset in samples
    val trimEnd: Int = -1,  // -1 means end of sample
    
    /**
     * Downsampled waveform data for visualization
     * 1000 points (float values)
     */
    val waveformData: ByteArray = byteArrayOf()
) {
    companion object {
        const val NUM_DRUM_PARTS = 8
        const val SYNTH_PART_INDEX = 8
        
        const val PITCH_MIN = -24.0f
        const val PITCH_MAX = 24.0f
        const val PAN_MIN = -1.0f
        const val PAN_MAX = 1.0f
        const val LEVEL_MIN = 0.0f
        const val LEVEL_MAX = 1.0f
        const val DECAY_MIN = 0.0f
        const val DECAY_MAX = 5000.0f
        
        const val FILTER_LPF = 0
        const val FILTER_HPF = 1
        
        const val WAVEFORM_POINTS = 1000
    }
}