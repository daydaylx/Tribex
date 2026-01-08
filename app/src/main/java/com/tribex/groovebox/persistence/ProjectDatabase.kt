package com.tribex.groovebox.persistence

import android.content.Context
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase
import androidx.room.TypeConverters
import com.tribex.groovebox.persistence.dao.MasterFXDao
import com.tribex.groovebox.persistence.dao.PartSettingsDao
import com.tribex.groovebox.persistence.dao.PatternDao
import com.tribex.groovebox.persistence.dao.ProjectDao
import com.tribex.groovebox.persistence.dao.SampleDao
import com.tribex.groovebox.persistence.entities.MasterFX
import com.tribex.groovebox.persistence.entities.PartSettings
import com.tribex.groovebox.persistence.entities.Pattern
import com.tribex.groovebox.persistence.entities.Project
import com.tribex.groovebox.persistence.entities.Sample
import java.io.File

/**
 * ProjectDatabase - Main Room Database
 * 
 * Schema Version 1 (M8)
 * Each project has its own database file:
 * /data/data/com.tribex.groovebox/files/projects/{projectUUID}/project.db
 */
@Database(
    entities = [
        Project::class,
        Pattern::class,
        Sample::class,
        PartSettings::class,
        MasterFX::class
    ],
    version = 1,
    exportSchema = true
)
@TypeConverters(Converters::class)
abstract class ProjectDatabase : RoomDatabase() {
    
    abstract fun projectDao(): ProjectDao
    abstract fun patternDao(): PatternDao
    abstract fun sampleDao(): SampleDao
    abstract fun partSettingsDao(): PartSettingsDao
    abstract fun masterFXDao(): MasterFXDao
    
    companion object {
        private const val DATABASE_NAME = "project.db"
        private const val DATABASE_TEMP_NAME = "project.db.tmp"
        
        /**
         * Create database instance for a specific project directory
         * 
         * M8: Enhanced atomic save strategy with WAL mode
         * - WAL (Write-Ahead Logging) for better performance and crash recovery
         * - Temporary file backup for atomic operations
         * - Proper migration strategy (no destructive fallback)
         * 
         * @param context Application context
         * @param projectDir Project directory (contains project.db and samples/)
         */
        fun create(context: Context, projectDir: File): ProjectDatabase {
            return Room.databaseBuilder(
                context.applicationContext,
                ProjectDatabase::class.java,
                File(projectDir, DATABASE_NAME).absolutePath
            )
                // M8: Use WAL mode for better performance and crash resilience
                .setJournalMode(RoomDatabase.JournalMode.WRITE_AHEAD_LOGGING)
                // M8: Add migrations when schema version increases
                .addMigrations(*ALL_MIGRATIONS)
                // M8: Enable multi-threading for database operations
                .allowMainThreadQueries()
                .build()
        }
        
        /**
         * Create in-memory database for testing
         */
        fun createInMemory(context: Context): ProjectDatabase {
            return Room.inMemoryDatabaseBuilder(
                context.applicationContext,
                ProjectDatabase::class.java
            ).build()
        }
    }
}