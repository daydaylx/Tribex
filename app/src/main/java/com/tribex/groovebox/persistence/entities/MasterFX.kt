package com.tribex.groovebox.persistence.entities

import androidx.room.Entity
import androidx.room.ForeignKey
import androidx.room.PrimaryKey

/**
 * MasterFX Entity - Stores master FX chain parameters
 * 
 * One row per project (projectId is primary key)
 * Contains all master FX parameters from M6 implementation
 */
@Entity(
    tableName = "master_fx",
    foreignKeys = [
        ForeignKey(
            entity = Project::class,
            parentColumns = ["id"],
            childColumns = ["projectId"],
            onDelete = ForeignKey.CASCADE
        )
    ]
)
data class MasterFX(
    @PrimaryKey
    val projectId: String,
    
    /**
     * Delay FX parameters
     */
    val delayTimeMs: Float = 300.0f,  // 0 to 1000ms
    val delayFeedback: Float = 0.4f,  // 0.0 to 0.9
    val delayMix: Float = 0.3f,  // 0.0 to 1.0
    
    /**
     * Reverb FX parameters
     */
    val reverbSize: Float = 0.7f,  // 0.0 to 1.0
    val reverbDensity: Float = 0.5f,  // 0.0 to 1.0
    val reverbMix: Float = 0.25f,  // 0.0 to 1.0
    
    /**
     * Valve Saturation parameters
     */
    val valveAmount: Float = 0.5f,  // 0.0 to 1.0
    
    /**
     * Limiter parameters
     */
    val limiterThresholdDb: Float = -3.0f,  // -12 to 0 dB
    val limiterReleaseMs: Float = 100.0f  // 10 to 1000ms
) {
    companion object {
        // Delay
        const val DELAY_TIME_MIN = 0.0f
        const val DELAY_TIME_MAX = 1000.0f
        const val DELAY_FEEDBACK_MIN = 0.0f
        const val DELAY_FEEDBACK_MAX = 0.9f
        const val DELAY_MIX_MIN = 0.0f
        const val DELAY_MIX_MAX = 1.0f
        
        // Reverb
        const val REVERB_SIZE_MIN = 0.0f
        const val REVERB_SIZE_MAX = 1.0f
        const val REVERB_DENSITY_MIN = 0.0f
        const val REVERB_DENSITY_MAX = 1.0f
        const val REVERB_MIX_MIN = 0.0f
        const val REVERB_MIX_MAX = 1.0f
        
        // Valve
        const val VALVE_AMOUNT_MIN = 0.0f
        const val VALVE_AMOUNT_MAX = 1.0f
        
        // Limiter
        const val LIMITER_THRESHOLD_MIN = -12.0f
        const val LIMITER_THRESHOLD_MAX = 0.0f
        const val LIMITER_RELEASE_MIN = 10.0f
        const val LIMITER_RELEASE_MAX = 1000.0f
    }
}