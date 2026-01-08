package com.tribex.groovebox.persistence.dao

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Update
import com.tribex.groovebox.persistence.entities.MasterFX
import kotlinx.coroutines.flow.Flow

/**
 * MasterFX DAO - Data Access Object for MasterFX entity
 */
@Dao
interface MasterFXDao {
    
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insert(masterFX: MasterFX): Long
    
    @Update
    suspend fun update(masterFX: MasterFX)
    
    @Query("DELETE FROM master_fx WHERE projectId = :projectId")
    suspend fun deleteByProjectId(projectId: String)
    
    @Query("SELECT * FROM master_fx WHERE projectId = :projectId")
    suspend fun getByProjectId(projectId: String): MasterFX?
    
    @Query("SELECT * FROM master_fx WHERE projectId = :projectId")
    fun getByProjectIdFlow(projectId: String): Flow<MasterFX?>
}