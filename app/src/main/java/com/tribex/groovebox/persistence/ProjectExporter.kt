package com.tribex.groovebox.persistence

import android.content.Context
import com.tribex.groovebox.persistence.dao.*
import com.tribex.groovebox.persistence.entities.*
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.Serializable
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json
import java.io.File
import java.io.FileOutputStream
import java.text.SimpleDateFormat
import java.util.*

/**
 * M8: Project Export and Backup
 * 
 * Provides:
 * - JSON export of pattern data, settings, and configuration
 * - CSV export of pattern matrices (for external tools)
 * - Full project backup (DB + samples copy)
 * 
 * Export Scope (per SPEC v3.1):
 * - Pattern data (gates, length, seed)
 * - Part settings (mute, solo, synth flag)
 * - Master FX settings
 * - Sample metadata (NOT WAV files - too large)
 */
class ProjectExporter(private val context: Context) {
    
    private val json = Json {
        ignoreUnknownKeys = true
        prettyPrint = true
    }
    
    private val projectsDir = File(context.filesDir, "projects")
    private val exportsDir = File(context.filesDir, "exports")
    
    init {
        // Ensure exports directory exists
        if (!exportsDir.exists()) {
            exportsDir.mkdirs()
        }
    }
    
    /**
     * Export project to JSON
     * 
     * Contains:
     * - Project metadata
     * - All patterns
     * - Part settings
     * - Master FX
     * - Sample metadata (not WAV files)
     * 
     * @param projectId UUID of the project to export
     * @return Result with export file path
     */
    suspend fun exportToJSON(projectId: String): Result<File> = withContext(Dispatchers.IO) {
        try {
            val projectDir = File(projectsDir, projectId)
            if (!projectDir.exists()) {
                return@withContext Result.failure(Exception("Project directory not found"))
            }
            
            // Open database
            val database = ProjectDatabase.create(context, projectDir)
            
            // Load all data
            val project = database.projectDao().getById(projectId)
                ?: return@withContext Result.failure(Exception("Project not found"))
            
            val patterns = database.patternDao().getAllForProject(projectId)
            val samples = database.sampleDao().getAllForProject(projectId)
            val partSettings = database.partSettingsDao().getAllForProject(projectId)
            val masterFX = database.masterFXDao().getByProjectId(projectId)
            
            database.close()
            
            // Create export data structure
            val exportData = ProjectExportData(
                version = 1,
                exportDate = SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'", Locale.US)
                    .format(Date()),
                project = project.toExportProject(),
                patterns = patterns.map { it.toExportPattern() },
                samples = samples.map { it.toExportSample() },
                partSettings = partSettings.map { it.toExportPartSettings() },
                masterFX = masterFX?.toExportMasterFX()
            )
            
            // Write to file
            val timestamp = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())
            val exportFile = File(exportsDir, "${project.name}_${timestamp}.json")
            
            FileOutputStream(exportFile).use { output ->
                output.write(json.encodeToString(exportData).toByteArray())
            }
            
            Result.success(exportFile)
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Export pattern to CSV
     * 
     * Format:
     * - Row headers: Pattern Index
     * - Column headers: Step Numbers
     * - Values: Part indices with gates (comma-separated)
     * 
     * Example:
     * Step,Part0,Part1,Part2,...,Part8
     * 0,1,0,1,...,0
     * 1,0,1,0,...,1
     * ...
     * 
     * @param projectId UUID of the project
     * @param patternIndex Pattern index to export (0-3)
     * @return Result with export file path
     */
    suspend fun exportToCSV(projectId: String, patternIndex: Int): Result<File> = withContext(Dispatchers.IO) {
        try {
            val projectDir = File(projectsDir, projectId)
            if (!projectDir.exists()) {
                return@withContext Result.failure(Exception("Project directory not found"))
            }
            
            // Open database
            val database = ProjectDatabase.create(context, projectDir)
            
            // Load project and pattern
            val project = database.projectDao().getById(projectId)
                ?: return@withContext Result.failure(Exception("Project not found"))
            
            val pattern = database.patternDao().getByProjectAndIndex(projectId, patternIndex)
                ?: return@withContext Result.failure(Exception("Pattern not found"))
            
            database.close()
            
            // Parse pattern steps
            val converter = Converters()
            val patternData = converter.toPatternData(pattern.steps)
            val length = pattern.length
            
            // Create CSV content
            val csv = StringBuilder()
            
            // Header
            csv.append("Step")
            for (partIndex in 0 until PartSettings.NUM_TOTAL_PARTS) {
                csv.append(",Part$partIndex")
            }
            csv.append("\n")
            
            // Data rows (each step)
            for (step in 0 until length) {
                csv.append(step)
                
                for (partIndex in 0 until PartSettings.NUM_TOTAL_PARTS) {
                    val gate = patternData.getGate(partIndex, step)
                    csv.append(",").append(if (gate) "1" else "0")
                }
                
                csv.append("\n")
            }
            
            // Write to file
            val timestamp = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())
            val exportFile = File(exportsDir, "${project.name}_pattern${patternIndex}_${timestamp}.csv")
            
            FileOutputStream(exportFile).use { output ->
                output.write(csv.toString().toByteArray())
            }
            
            Result.success(exportFile)
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Create full project backup
     * 
     * Creates a ZIP archive containing:
     * - project.db (database)
     * - samples/ directory (WAV files)
     * 
     * @param projectId UUID of the project
     * @return Result with backup file path
     */
    suspend fun createBackup(projectId: String): Result<File> = withContext(Dispatchers.IO) {
        try {
            val projectDir = File(projectsDir, projectId)
            if (!projectDir.exists()) {
                return@withContext Result.failure(Exception("Project directory not found"))
            }
            
            // Load project name
            val database = ProjectDatabase.create(context, projectDir)
            val project = database.projectDao().getById(projectId)
                ?: return@withContext Result.failure(Exception("Project not found"))
            database.close()
            
            // Create backup directory
            val timestamp = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())
            val backupDir = File(exportsDir, "${project.name}_backup_${timestamp}")
            backupDir.mkdirs()
            
            // Copy project.db
            val dbSource = File(projectDir, "project.db")
            val dbDest = File(backupDir, "project.db")
            dbSource.copyTo(dbDest, overwrite = true)
            
            // Copy samples directory
            val samplesSource = File(projectDir, "samples")
            if (samplesSource.exists()) {
                val samplesDest = File(backupDir, "samples")
                samplesDest.mkdirs()
                
                samplesSource.listFiles()?.forEach { file ->
                    file.copyTo(File(samplesDest, file.name), overwrite = true)
                }
            }
            
            // Create README
            val readme = File(backupDir, "README.txt")
            FileOutputStream(readme).use { output ->
                output.write(
                    """
                    TribeX Project Backup
                    
                    Project: ${project.name}
                    Project ID: ${project.id}
                    Backup Date: ${SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.US).format(Date())}
                    
                    Contents:
                    - project.db: SQLite database with patterns, settings, and metadata
                    - samples/: WAV files for drum samples
                    
                    To restore:
                    1. Copy this folder to /files/projects/
                    2. Rename folder to project UUID
                    3. Restart TribeX
                    """.trimIndent().toByteArray()
                )
            }
            
            Result.success(backupDir)
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Get list of all exports
     */
    fun getExports(): List<File> {
        return exportsDir.listFiles()?.toList() ?: emptyList()
    }
    
    /**
     * Delete an export
     */
    fun deleteExport(file: File): Result<Unit> {
        return try {
            if (file.parentFile?.absolutePath == exportsDir.absolutePath) {
                file.deleteRecursively()
                Result.success(Unit)
            } else {
                Result.failure(Exception("File is not in exports directory"))
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
}

/**
 * Serializable export data structure for JSON export
 */
@Serializable
data class ProjectExportData(
    val version: Int,
    val exportDate: String,
    val project: ExportProject,
    val patterns: List<ExportPattern>,
    val samples: List<ExportSample>,
    val partSettings: List<ExportPartSettings>,
    val masterFX: ExportMasterFX?
)

/**
 * Sample metadata for export (without WAV file)
 */
@Serializable
data class ExportSample(
    val id: String,
    val projectId: String,
    val partIndex: Int,
    val filename: String,
    val originalPath: String?,
    val pitch: Float,
    val pan: Float,
    val level: Float,
    val decay: Float,
    val filterType: Int,
    val trimStart: Int,
    val trimEnd: Int
)

/**
 * Project metadata for export
 */
@Serializable
data class ExportProject(
    val id: String,
    val name: String,
    val createdAt: Long,
    val lastModified: Long
)

/**
 * Pattern for export
 */
@Serializable
data class ExportPattern(
    val id: String,
    val projectId: String,
    val patternIndex: Int,
    val steps: String,
    val length: Int,
    val seed: Int
)

/**
 * Part settings for export
 */
@Serializable
data class ExportPartSettings(
    val id: String,
    val projectId: String,
    val partIndex: Int,
    val muted: Boolean,
    val soloed: Boolean,
    val isSynth: Boolean
)

/**
 * Master FX for export
 */
@Serializable
data class ExportMasterFX(
    val projectId: String,
    val delayTimeMs: Float,
    val delayFeedback: Float,
    val delayMix: Float,
    val reverbSize: Float,
    val reverbDensity: Float,
    val reverbMix: Float,
    val valveAmount: Float,
    val limiterThresholdDb: Float,
    val limiterReleaseMs: Float
)

/**
 * Extension to convert Sample entity to ExportSample
 */
fun Sample.toExportSample(): ExportSample {
    return ExportSample(
        id = id,
        projectId = projectId,
        partIndex = partIndex,
        filename = filename,
        originalPath = originalPath,
        pitch = pitch,
        pan = pan,
        level = level,
        decay = decay,
        filterType = filterType,
        trimStart = trimStart,
        trimEnd = trimEnd
    )
}

/**
 * Extension to convert Project entity to ExportProject
 */
fun Project.toExportProject(): ExportProject {
    return ExportProject(
        id = id,
        name = name,
        createdAt = createdAt,
        lastModified = lastModified
    )
}

/**
 * Extension to convert Pattern entity to ExportPattern
 */
fun Pattern.toExportPattern(): ExportPattern {
    return ExportPattern(
        id = id,
        projectId = projectId,
        patternIndex = patternIndex,
        steps = steps,
        length = length,
        seed = seed
    )
}

/**
 * Extension to convert PartSettings entity to ExportPartSettings
 */
fun PartSettings.toExportPartSettings(): ExportPartSettings {
    return ExportPartSettings(
        id = id,
        projectId = projectId,
        partIndex = partIndex,
        muted = muted,
        soloed = soloed,
        isSynth = isSynth
    )
}

/**
 * Extension to convert MasterFX entity to ExportMasterFX
 */
fun MasterFX.toExportMasterFX(): ExportMasterFX {
    return ExportMasterFX(
        projectId = projectId,
        delayTimeMs = delayTimeMs,
        delayFeedback = delayFeedback,
        delayMix = delayMix,
        reverbSize = reverbSize,
        reverbDensity = reverbDensity,
        reverbMix = reverbMix,
        valveAmount = valveAmount,
        limiterThresholdDb = limiterThresholdDb,
        limiterReleaseMs = limiterReleaseMs
    )
}