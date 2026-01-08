package com.tribex.groovebox.persistence.dao

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Update
import com.tribex.groovebox.persistence.entities.Pattern
import kotlinx.coroutines.flow.Flow

/**
 * Pattern DAO - Data Access Object for Pattern entity
 */
@Dao
interface PatternDao {
    
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insert(pattern: Pattern): Long
    
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertAll(patterns: List<Pattern>)
    
    @Update
    suspend fun update(pattern: Pattern)
    
    @Query("DELETE FROM patterns WHERE id = :patternId")
    suspend fun deleteById(patternId: String)
    
    @Query("DELETE FROM patterns WHERE projectId = :projectId")
    suspend fun deleteAllForProject(projectId: String)
    
    @Query("SELECT * FROM patterns WHERE id = :patternId")
    suspend fun getById(patternId: String): Pattern?
    
    @Query("SELECT * FROM patterns WHERE projectId = :projectId AND patternIndex = :patternIndex")
    suspend fun getByProjectAndIndex(projectId: String, patternIndex: Int): Pattern?
    
    @Query("SELECT * FROM patterns WHERE projectId = :projectId ORDER BY patternIndex ASC")
    suspend fun getAllForProject(projectId: String): List<Pattern>
    
    @Query("SELECT * FROM patterns WHERE projectId = :projectId ORDER BY patternIndex ASC")
    fun getAllForProjectFlow(projectId: String): Flow<List<Pattern>>
}