package com.tribex.groovebox.persistence.dao

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Update
import com.tribex.groovebox.persistence.entities.PartSettings
import kotlinx.coroutines.flow.Flow

/**
 * PartSettings DAO - Data Access Object for PartSettings entity
 */
@Dao
interface PartSettingsDao {
    
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insert(partSettings: PartSettings): Long
    
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertAll(partSettingsList: List<PartSettings>)
    
    @Update
    suspend fun update(partSettings: PartSettings)
    
    @Query("DELETE FROM part_settings WHERE id = :settingsId")
    suspend fun deleteById(settingsId: String)
    
    @Query("DELETE FROM part_settings WHERE projectId = :projectId")
    suspend fun deleteAllForProject(projectId: String)
    
    @Query("SELECT * FROM part_settings WHERE id = :settingsId")
    suspend fun getById(settingsId: String): PartSettings?
    
    @Query("SELECT * FROM part_settings WHERE projectId = :projectId AND partIndex = :partIndex")
    suspend fun getByProjectAndPart(projectId: String, partIndex: Int): PartSettings?
    
    @Query("SELECT * FROM part_settings WHERE projectId = :projectId ORDER BY partIndex ASC")
    suspend fun getAllForProject(projectId: String): List<PartSettings>
    
    @Query("SELECT * FROM part_settings WHERE projectId = :projectId ORDER BY partIndex ASC")
    fun getAllForProjectFlow(projectId: String): Flow<List<PartSettings>>}