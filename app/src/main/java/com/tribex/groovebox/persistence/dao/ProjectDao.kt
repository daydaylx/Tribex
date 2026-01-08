package com.tribex.groovebox.persistence.dao

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Update
import com.tribex.groovebox.persistence.entities.Project
import kotlinx.coroutines.flow.Flow

/**
 * Project DAO - Data Access Object for Project entity
 */
@Dao
interface ProjectDao {
    
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insert(project: Project): Long
    
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertAll(projects: List<Project>)
    
    @Update
    suspend fun update(project: Project)
    
    @Query("DELETE FROM projects WHERE id = :projectId")
    suspend fun deleteById(projectId: String)
    
    @Query("SELECT * FROM projects WHERE id = :projectId")
    suspend fun getById(projectId: String): Project?
    
    @Query("SELECT * FROM projects WHERE id = :projectId")
    fun getByIdFlow(projectId: String): Flow<Project?>
    
    @Query("SELECT * FROM projects ORDER BY lastModified DESC LIMIT 1")
    suspend fun getLastModified(): Project?
    
    @Query("SELECT * FROM projects ORDER BY name ASC")
    fun getAllProjects(): Flow<List<Project>>
    
    @Query("UPDATE projects SET lastModified = :timestamp WHERE id = :projectId")
    suspend fun updateLastModified(projectId: String, timestamp: Long)
}