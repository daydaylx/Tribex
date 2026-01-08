package com.tribex.groovebox.persistence.entities

import androidx.room.Entity
import androidx.room.ForeignKey
import androidx.room.Index
import androidx.room.PrimaryKey

/**
 * Pattern Entity - Stores pattern step data for a project
 * 
 * Pattern index: 0-3 for 4 patterns per project
 * Steps stored as JSON string for 16-64 steps × 9 parts
 * Each step contains: gate, velocity, microtiming, probability, locks
 */
@Entity(
    tableName = "patterns",
    foreignKeys = [
        ForeignKey(
            entity = Project::class,
            parentColumns = ["id"],
            childColumns = ["projectId"],
            onDelete = ForeignKey.CASCADE
        )
    ],
    indices = [
        Index(value = ["projectId"])
    ]
)
data class Pattern(
    @PrimaryKey
    val id: String,
    
    val projectId: String,
    
    /**
     * Pattern index (0-3 for 4 patterns)
     */
    val patternIndex: Int,  // 0, 1, 2, 3
    
    /**
     * Steps data as JSON
     * Structure: Array of steps (16, 32, or 64)
     * Each step: Map of partIndex to step data
     * Part 0-7: Drum parts
     * Part 8: Synth part
     * 
     * Step data format (JSON):
     * {
     *   "gate": boolean,
     *   "velocity": int (0-127),
     *   "microtiming": int (-127 to 127),
     *   "probability": int (0-100),
     *   "locks": Map<parameter, value>  // Optional
     * }
     */
    val steps: String,  // JSON string
    
    /**
     * Pattern length in steps (16, 32, or 64)
     */
    val length: Int = 16,
    
    /**
     * Seed for probability determinism
     * Ensures same playback on restart and export
     */
    val seed: Int = 0
) {
    companion object {
        const val MIN_LENGTH = 16
        const val MAX_LENGTH = 64
        const val DEFAULT_LENGTH = 16
        const val NUM_PATTERNS = 4
    }
}