package com.tribex.groovebox.persistence

import android.content.Context
import android.content.SharedPreferences
import com.tribex.groovebox.persistence.dao.MasterFXDao
import com.tribex.groovebox.persistence.dao.PartSettingsDao
import com.tribex.groovebox.persistence.dao.PatternDao
import com.tribex.groovebox.persistence.dao.ProjectDao
import com.tribex.groovebox.persistence.dao.SampleDao
import com.tribex.groovebox.persistence.entities.*
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.withContext
import java.io.File
import java.util.UUID

/**
 * ProjectManager - Singleton for managing TribeX projects
 * 
 * Handles:
 * - Project creation and loading
 * - Atomic save strategy
 * - File system operations (project.db and samples/)
 * - Autosave triggering
 */
class ProjectManager private constructor(private val context: Context) {
    
    private val projectsDir: File = File(context.filesDir, "projects")
    private val prefs: SharedPreferences = context.getSharedPreferences("tribex_prefs", Context.MODE_PRIVATE)
    
    private var currentProject: Project? = null
    private var currentDatabase: ProjectDatabase? = null
    
    // DAOs for current project
    private var projectDao: ProjectDao? = null
    private var patternDao: PatternDao? = null
    private var sampleDao: SampleDao? = null
    private var partSettingsDao: PartSettingsDao? = null
    private var masterFXDao: MasterFXDao? = null
    
    companion object {
        private const val PREF_LAST_PROJECT_ID = "last_project_id"
        private const val PREF_AUTO_SAVE_ENABLED = "auto_save_enabled"
        
        @Volatile
        private var instance: ProjectManager? = null
        
        fun getInstance(context: Context): ProjectManager {
            return instance ?: synchronized(this) {
                instance ?: ProjectManager(context.applicationContext).also { instance = it }
            }
        }
    }
    
    init {
        // Ensure projects directory exists
        if (!projectsDir.exists()) {
            projectsDir.mkdirs()
        }
    }
    
    /**
     * Get all projects
     */
    fun getAllProjects(): List<File> {
        return projectsDir.listFiles { file ->
            file.isDirectory && File(file, "project.db").exists()
        }?.toList() ?: emptyList()
    }
    
    /**
     * Create a new project
     */
    suspend fun createProject(name: String = "Untitled Project"): Result<Project> = withContext(Dispatchers.IO) {
        try {
            val projectId = UUID.randomUUID().toString()
            val projectDir = File(projectsDir, projectId)
            
            // Create project directory
            if (!projectDir.exists()) {
                projectDir.mkdirs()
            }
            
            // Create samples directory
            val samplesDir = File(projectDir, "samples")
            samplesDir.mkdirs()
            
            // Create database
            val database = ProjectDatabase.create(context, projectDir)
            
            // Insert project entity
            val project = Project(
                id = projectId,
                name = name
            )
            database.projectDao().insert(project)
            
            // Create default patterns (4 patterns with 16 steps each)
            createDefaultPatterns(projectId, database.patternDao())
            
            // Create default part settings (9 parts)
            createDefaultPartSettings(projectId, database.partSettingsDao())
            
            // Create default master FX
            createDefaultMasterFX(projectId, database.masterFXDao())
            
            database.close()
            
            Result.success(project)
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Load a project by ID
     */
    suspend fun loadProject(projectId: String): Result<Project> = withContext(Dispatchers.IO) {
        try {
            // Close current database if open
            currentDatabase?.close()
            
            // Get project directory
            val projectDir = File(projectsDir, projectId)
            if (!projectDir.exists()) {
                return@withContext Result.failure(Exception("Project directory not found"))
            }
            
            // Create database instance
            val database = ProjectDatabase.create(context, projectDir)
            
            // Load project entity
            val project = database.projectDao().getById(projectId)
                ?: return@withContext Result.failure(Exception("Project not found in database"))
            
            // Set current project and database
            currentProject = project
            currentDatabase = database
            projectDao = database.projectDao()
            patternDao = database.patternDao()
            sampleDao = database.sampleDao()
            partSettingsDao = database.partSettingsDao()
            masterFXDao = database.masterFXDao()
            
            // Save as last project
            prefs.edit().putString(PREF_LAST_PROJECT_ID, projectId).apply()
            
            Result.success(project)
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Load last project (if exists)
     */
    suspend fun loadLastProject(): Result<Project?> = withContext(Dispatchers.IO) {
        try {
            val lastProjectId = prefs.getString(PREF_LAST_PROJECT_ID, null)
            if (lastProjectId != null) {
                loadProject(lastProjectId)
            } else {
                Result.success(null)
            }
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Save current project (atomic)
     */
    suspend fun saveCurrentProject(): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            val project = currentProject
                ?: return@withContext Result.failure(Exception("No project loaded"))
            
            val database = currentDatabase
                ?: return@withContext Result.failure(Exception("No database open"))
            
            // Update last modified timestamp
            val updatedProject = project.copy(lastModified = System.currentTimeMillis())
            database.projectDao().update(updatedProject)
            currentProject = updatedProject
            
            Result.success(Unit)
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * M8: Atomic save strategy with temporary file backup
     * 
     * Strategy:
     * 1. Update database with Room transactions (atomic)
     * 2. Close database (ensures WAL is flushed)
     * 3. Create backup of DB file to temporary location
     * 4. Verify backup integrity
     * 5. Reopen database
     * 
     * Note: WAL mode provides automatic crash recovery.
     * This method adds an extra layer of protection.
     */
    suspend fun saveCurrentProjectAtomic(): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            val project = currentProject
                ?: return@withContext Result.failure(Exception("No project loaded"))
            
            val database = currentDatabase
                ?: return@withContext Result.failure(Exception("No database open"))
            
            val projectDir = File(projectsDir, project.id)
            val dbFile = File(projectDir, "project.db")
            val tempDbFile = File(projectDir, "project.db.tmp")
            
            // Update last modified timestamp
            val updatedProject = project.copy(lastModified = System.currentTimeMillis())
            database.projectDao().update(updatedProject)
            currentProject = updatedProject
            
            // Close database to ensure WAL is flushed to disk
            database.close()
            
            // Create backup of DB file
            if (dbFile.exists()) {
                dbFile.copyTo(tempDbFile, overwrite = true)
                
                // Verify backup exists and is readable
                if (!tempDbFile.exists() || tempDbFile.length() == 0L) {
                    return@withContext Result.failure(Exception("Backup failed - temporary file is empty"))
                }
            }
            
            // Reopen database
            val newDatabase = ProjectDatabase.create(context, projectDir)
            currentDatabase = newDatabase
            projectDao = newDatabase.projectDao()
            patternDao = newDatabase.patternDao()
            sampleDao = newDatabase.sampleDao()
            partSettingsDao = newDatabase.partSettingsDao()
            masterFXDao = newDatabase.masterFXDao()
            
            Result.success(Unit)
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Autosave current project (called on screen switch / app pause)
     */
    suspend fun autosave(): Result<Unit> {
        return saveCurrentProjectAtomic()
    }
    
    /**
     * Get current project
     */
    fun getCurrentProject(): Project? = currentProject
    
    /**
     * Get project DAO for current project
     */
    fun getProjectDao(): ProjectDao? = projectDao
    
    /**
     * Get pattern DAO for current project
     */
    fun getPatternDao(): PatternDao? = patternDao
    
    /**
     * Get sample DAO for current project
     */
    fun getSampleDao(): SampleDao? = sampleDao
    
    /**
     * Get part settings DAO for current project
     */
    fun getPartSettingsDao(): PartSettingsDao? = partSettingsDao
    
    /**
     * Get master FX DAO for current project
     */
    fun getMasterFXDao(): MasterFXDao? = masterFXDao
    
    /**
     * Get samples directory for current project
     */
    fun getSamplesDir(): File? {
        val projectId = currentProject?.id ?: return null
        return File(projectsDir, projectId).let { File(it, "samples") }
    }
    
    /**
     * Delete a project
     */
    suspend fun deleteProject(projectId: String): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            // Close database if it's the current project
            if (currentProject?.id == projectId) {
                currentDatabase?.close()
                currentProject = null
                currentDatabase = null
                projectDao = null
                patternDao = null
                sampleDao = null
                partSettingsDao = null
                masterFXDao = null
                
                // Clear last project preference
                if (prefs.getString(PREF_LAST_PROJECT_ID, null) == projectId) {
                    prefs.edit().remove(PREF_LAST_PROJECT_ID).apply()
                }
            }
            
            // Delete project directory (recursive)
            val projectDir = File(projectsDir, projectId)
            if (projectDir.exists()) {
                projectDir.deleteRecursively()
            }
            
            Result.success(Unit)
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Rename a project
     */
    suspend fun renameProject(projectId: String, newName: String): Result<Unit> = withContext(Dispatchers.IO) {
        try {
            // If it's the current project, update memory
            if (currentProject?.id == projectId) {
                val updatedProject = currentProject!!.copy(name = newName)
                currentDatabase?.projectDao()?.update(updatedProject)
                currentProject = updatedProject
            } else {
                // Load database, update, close
                val projectDir = File(projectsDir, projectId)
                val database = ProjectDatabase.create(context, projectDir)
                val project = database.projectDao().getById(projectId)
                if (project != null) {
                    database.projectDao().update(project.copy(name = newName))
                }
                database.close()
            }
            
            Result.success(Unit)
        } catch (e: Exception) {
            Result.failure(e)
        }
    }
    
    /**
     * Create default patterns for a new project
     */
    private suspend fun createDefaultPatterns(projectId: String, patternDao: PatternDao) {
        repeat(Pattern.NUM_PATTERNS) { patternIndex ->
            val pattern = Pattern(
                id = UUID.randomUUID().toString(),
                projectId = projectId,
                patternIndex = patternIndex,
                steps = "{}",  // Empty steps (no gates set)
                length = Pattern.DEFAULT_LENGTH,
                seed = 0
            )
            patternDao.insert(pattern)
        }
    }
    
    /**
     * Create default part settings for a new project
     */
    private suspend fun createDefaultPartSettings(projectId: String, partSettingsDao: PartSettingsDao) {
        repeat(PartSettings.NUM_TOTAL_PARTS) { partIndex ->
            val partSettings = PartSettings(
                id = UUID.randomUUID().toString(),
                projectId = projectId,
                partIndex = partIndex,
                muted = false,
                soloed = false,
                isSynth = partIndex == Sample.SYNTH_PART_INDEX
            )
            partSettingsDao.insert(partSettings)
        }
    }
    
    /**
     * Create default master FX for a new project
     */
    private suspend fun createDefaultMasterFX(projectId: String, masterFXDao: MasterFXDao) {
        val masterFX = MasterFX(projectId = projectId)
        masterFXDao.insert(masterFX)
    }
}