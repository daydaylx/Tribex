package com.tribex.groovebox.persistence.entities

import androidx.room.Entity
import androidx.room.PrimaryKey
import java.util.UUID

/**
 * Project Entity - Represents a TribeX project
 * 
 * Each project is stored in its own directory:
 * /data/data/com.tribex.groovebox/files/projects/{projectUUID}/
 *   - project.db (this entity + patterns + samples + settings)
 *   - samples/ (WAV files)
 */
@Entity(tableName = "projects")
data class Project(
    @PrimaryKey
    val id: String = UUID.randomUUID().toString(),
    
    val name: String = "Untitled Project",
    
    val bpm: Float = 120.0f,
    
    val createdAt: Long = System.currentTimeMillis(),
    
    val lastModified: Long = System.currentTimeMillis()
) {
    companion object {
        const val DEFAULT_BPM = 120.0f
    }
}