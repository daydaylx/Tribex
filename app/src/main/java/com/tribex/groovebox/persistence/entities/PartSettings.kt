package com.tribex.groovebox.persistence.entities

import androidx.room.Entity
import androidx.room.ForeignKey
import androidx.room.PrimaryKey

/**
 * PartSettings Entity - Stores mute/solo state for each part
 * 
 * Part index: 0-8 (9 total parts)
 * Separate from Sample entity to handle synth part (part 8)
 */
@Entity(
    tableName = "part_settings",
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
data class PartSettings(
    @PrimaryKey
    val id: String,
    
    val projectId: String,
    
    /**
     * Part index (0-8 for all parts)
     * 0-7: Drum parts
     * 8: Synth part
     */
    val partIndex: Int,  // 0-8
    
    /**
     * Mute state
     */
    val muted: Boolean = false,
    
    /**
     * Solo state
     */
    val soloed: Boolean = false,
    
    /**
     * Is this a synth part?
     */
    val isSynth: Boolean = false
) {
    companion object {
        const val NUM_TOTAL_PARTS = 9
    }
}