package com.tribex.groovebox.persistence

import android.content.Context
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.tribex.groovebox.persistence.dao.PatternDao
import com.tribex.groovebox.persistence.dao.ProjectDao
import com.tribex.groovebox.persistence.entities.Pattern
import com.tribex.groovebox.persistence.entities.Project
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import java.io.IOException
import java.util.UUID

@RunWith(AndroidJUnit4::class)
class DatabaseTest {
    private lateinit var projectDao: ProjectDao
    private lateinit var patternDao: PatternDao
    private lateinit var db: ProjectDatabase

    @Before
    fun createDb() {
        val context = ApplicationProvider.getApplicationContext<Context>()
        db = ProjectDatabase.createInMemory(context)
        projectDao = db.projectDao()
        patternDao = db.patternDao()
    }

    @After
    @Throws(IOException::class)
    fun closeDb() {
        db.close()
    }

    @Test
    @Throws(Exception::class)
    fun writeProjectAndReadBack() = runTest {
        val projectId = UUID.randomUUID().toString()
        val project = Project(id = projectId, name = "Test Project", bpm = 140f)
        projectDao.insert(project)
        
        val lastProject = projectDao.getLastModified()
        assertNotNull(lastProject)
        assertEquals("Test Project", lastProject?.name)
        assertEquals(140f, lastProject?.bpm ?: 0f)
    }

    @Test
    @Throws(Exception::class)
    fun writeProjectWithPatternsAndReadBack() = runTest {
        val projectId = UUID.randomUUID().toString()
        val project = Project(id = projectId, name = "Pattern Test")
        projectDao.insert(project)
        
        val patternId = UUID.randomUUID().toString()
        val pattern = Pattern(
            id = patternId,
            projectId = projectId,
            patternIndex = 0,
            steps = "[]",
            length = 16,
            seed = 123
        )
        patternDao.insert(pattern)
        
        val retrievedPattern = patternDao.getByProjectAndIndex(projectId, 0)
        assertNotNull(retrievedPattern)
        assertEquals(patternId, retrievedPattern?.id)
        assertEquals(123, retrievedPattern?.seed)
    }
}
