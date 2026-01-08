package com.tribex.groovebox.persistence

import androidx.room.migration.Migration
import androidx.sqlite.db.SupportSQLiteDatabase

/**
 * M8: Database Migration Strategy
 * 
 * Schema Version History:
 * - Version 1: Initial schema (Project, Pattern, Sample, PartSettings, MasterFX)
 * 
 * Future migrations should be added here as the schema evolves.
 * Each migration must handle data transformation from old to new schema.
 * 
 * Best practices:
 * - Always provide fallback for missing columns
 * - Use transactions for atomic migrations
 * - Test migrations with real data before deployment
 */

/**
 * Migration from Version 1 to Version 2
 * 
 * Example migration (placeholder for future schema changes):
 * - Add new column to existing table
 * - Modify column type
 * - Create new tables
 * 
 * Note: This is a placeholder. Add actual migration logic when schema changes.
 */
val MIGRATION_1_2 = object : Migration(1, 2) {
    override fun migrate(database: SupportSQLiteDatabase) {
        // Example: Add a new column to Project table
        // database.execSQL("ALTER TABLE projects ADD COLUMN tempo INTEGER NOT NULL DEFAULT 120")
        
        // Example: Create new table
        // database.execSQL("""
        //     CREATE TABLE IF NOT EXISTS new_table (
        //         id TEXT PRIMARY KEY NOT NULL,
        //         projectId TEXT NOT NULL,
        //         FOREIGN KEY(projectId) REFERENCES projects(id) ON DELETE CASCADE
        //     )
        // """)
        
        // Note: No schema changes in M8, this migration is prepared for future use
    }
}

/**
 * Get all migrations for the database
 * 
 * Returns array of Migration objects to pass to Room database builder
 */
val ALL_MIGRATIONS: Array<Migration>
    get() = arrayOf(
        // MIGRATION_1_2 will be added here when needed
        // MIGRATION_2_3 will be added here when needed
        // ...
    )