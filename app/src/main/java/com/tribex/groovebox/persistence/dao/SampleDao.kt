package com.tribex.groovebox.persistence.dao

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Update
import com.tribex.groovebox.persistence.entities.Sample
import kotlinx.coroutines.flow.Flow

/**
 * Sample DAO - Data Access Object for Sample entity
 */
@Dao
interface SampleDao {
    
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insert(sample: Sample): Long
    
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertAll(samples: List<Sample>)
    
    @Update
    suspend fun update(sample: Sample)
    
    @Query("DELETE FROM samples WHERE id = :sampleId")
    suspend fun deleteById(sampleId: String)
    
    @Query("DELETE FROM samples WHERE projectId = :projectId")
    suspend fun deleteAllForProject(projectId: String)
    
    @Query("DELETE FROM samples WHERE projectId = :projectId AND partIndex = :partIndex")
    suspend fun deleteForProjectAndPart(projectId: String, partIndex: Int)
    
    @Query("SELECT * FROM samples WHERE id = :sampleId")
    suspend fun getById(sampleId: String): Sample?
    
    @Query("SELECT * FROM samples WHERE projectId = :projectId AND partIndex = :partIndex")
    suspend fun getByProjectAndPart(projectId: String, partIndex: Int): Sample?
    
    @Query("SELECT * FROM samples WHERE projectId = :projectId ORDER BY partIndex ASC")
    suspend fun getAllForProject(projectId: String): List<Sample>
    
    @Query("SELECT * FROM samples WHERE projectId = :projectId ORDER BY partIndex ASC")
    fun getAllForProjectFlow(projectId: String): Flow<List<Sample>>
}