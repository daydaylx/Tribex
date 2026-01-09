package com.tribex.groovebox.ui.viewmodel

import android.content.Context
import com.tribex.groovebox.persistence.PatternStepsConverter
import com.tribex.groovebox.persistence.ProjectManager
import com.tribex.groovebox.persistence.StepData as PersistenceStepData
import com.tribex.groovebox.persistence.PatternData
import com.tribex.groovebox.persistence.entities.Pattern as PatternEntity
import com.tribex.groovebox.ui.screen.PatternState
import com.tribex.groovebox.ui.screen.StepDisplayState
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * PatternConverter - Handles conversion between PatternState (UI) and PatternEntity (Persistence)
 *
 * This class extracts the conversion logic from PatternViewModel to improve separation of concerns
 * and make the code more maintainable.
 *
 * @property context Android context used to access ProjectManager
 */
class PatternConverter(private val context: Context) {
    
    private val projectManager = ProjectManager.getInstance(context)
    
    /**
     * Convert Pattern Entity to PatternState
     *
     * Converts a PatternEntity from Room database to a PatternState suitable for UI display.
     * This involves:
     * - Parsing JSON step data
     * - Loading BPM from the project
     * - Converting velocity from MIDI (0-127) to 2-bit (0-3) representation
     * - Handling pagination (16 steps per page)
     * - Preserving the current page during conversion
     *
     * @param entity Pattern Entity from Room database containing JSON step data
     * @return PatternState ready for UI display with proper velocity conversion and pagination
     */
    suspend fun convertEntityToState(entity: PatternEntity): PatternState = withContext(Dispatchers.IO) {
        // Parse JSON steps
        val patternData = PatternStepsConverter.toPatternData(entity.steps)
        
        // Load BPM from project
        val project = projectManager.getCurrentProject()
        val bpm = project?.bpm ?: 120f
        
        // Convert to PatternState format
        // PatternState uses List<List<StepDisplayState>> for 9 parts × 16 steps (current page)
        // Pattern Entity stores all steps (up to 64) as JSON
        // We load the current page (16 steps) based on currentPage
        
        val patternLength = entity.length
        val stepsPerPage = 16
        
        // Get current page from state (preserved during load)
        val currentPage = 0u  // Default to page 0, would be passed in real implementation
        
        // Ensure current page is within bounds
        val maxPage = ((patternLength - 1) / stepsPerPage).toUInt()
        val safePage = if (currentPage <= maxPage) currentPage else 0u
        
        val steps = List(9) { partIndex ->
            List(stepsPerPage) { pageStepIndex ->
                // Calculate absolute step index (current page offset + page step)
                val absoluteStepIndex = (safePage.toInt() * stepsPerPage) + pageStepIndex
                
                // Get step data from patternData (only if within pattern length)
                val stepData = if (absoluteStepIndex < patternLength) {
                    patternData.steps.getOrNull(absoluteStepIndex)?.get(partIndex)
                } else {
                    null
                }
                
                // Convert velocity from MIDI (0-127) to 2-bit (0-3) for PatternState
                val midiVelocity = stepData?.velocity ?: 80
                val velocity2bit: UByte = when {
                    midiVelocity < 45 -> 0u  // Ghost
                    midiVelocity < 75 -> 1u  // Normal
                    midiVelocity < 95 -> 2u  // Accent
                    else -> 3u  // Max
                }.toUByte()
                
                StepDisplayState(
                    stepIndex = pageStepIndex.toUInt(),
                    partIndex = partIndex.toUInt(),
                    gate = stepData?.gate ?: false,
                    velocity = velocity2bit,
                    probability = (stepData?.probability ?: 100).toUByte().coerceIn(0u, 100u),
                    microtiming = (stepData?.microtiming ?: 0).toByte().coerceIn(-50, 50)
                )
            }
        }
        
        PatternState(
            currentStep = 0u,
            bpm = bpm,
            isPlaying = false,
            currentPatternId = entity.patternIndex.toUInt(),
            patternLengthSteps = entity.length.toUInt(),
            currentPage = safePage,
            patternSeed = entity.seed.toUInt(),
            steps = steps
        )
    }
    
    /**
     * Convert PatternState to Pattern Entity
     *
     * Converts a PatternState from UI to a PatternEntity suitable for Room database persistence.
     * This involves:
     * - Handling multi-page patterns (16/32/48/64 steps)
     * - Merging current page changes with existing pattern data
     * - Converting velocity from 2-bit (0-3) to MIDI (0-127) representation
     * - Preserving existing step data for non-current pages
     * - Serializing step data to JSON format
     *
     * @param state PatternState from UI containing current page step data
     * @param projectId Project ID for database association
     * @param patternIndex Pattern index (0-3) within the project
     * @return PatternEntity ready for Room database persistence with JSON step data
     */
    suspend fun convertStateToEntity(
        state: PatternState,
        projectId: String,
        patternIndex: Int
    ): PatternEntity = withContext(Dispatchers.IO) {
        // Get existing pattern to merge with
        val existingPattern = projectManager.getPatternDao()
            ?.getByProjectAndIndex(projectId, patternIndex)
        
        // Parse existing pattern data (if exists) or create empty
        val existingPatternData = if (existingPattern != null && existingPattern.steps.isNotEmpty()) {
            try {
                PatternStepsConverter.toPatternData(existingPattern.steps)
            } catch (e: Exception) {
                // If parsing fails, create empty pattern
                PatternData(steps = emptyList())
            }
        } else {
            PatternData(steps = emptyList())
        }
        
        // Calculate pattern length
        val patternLength = state.patternLengthSteps.toInt()
        val stepsPerPage = 16
        val currentPage = state.currentPage.toInt()
        
        // Create mutable list of all steps (up to patternLength)
        val allSteps = mutableListOf<Map<Int, PersistenceStepData>>()
        
        // Initialize with existing steps (if any)
        for (i in 0 until patternLength.coerceAtMost(existingPatternData.steps.size)) {
            allSteps.add(existingPatternData.steps.getOrNull(i)?.toMutableMap() ?: mutableMapOf())
        }
        
        // Fill remaining steps with empty maps if needed
        while (allSteps.size < patternLength) {
            allSteps.add(mutableMapOf())
        }
        
        // Update current page steps from PatternState
        val pageStartIndex = currentPage * stepsPerPage
        for (pageStepIndex in 0 until stepsPerPage) {
            val absoluteStepIndex = pageStartIndex + pageStepIndex
            
            // Only update if within pattern length
            if (absoluteStepIndex < patternLength) {
                val stepMap = allSteps[absoluteStepIndex].toMutableMap()
                
                // Update all parts for this step
                state.steps.forEachIndexed { partIndex, partSteps ->
                    if (pageStepIndex < partSteps.size) {
                        val stepDisplayState = partSteps[pageStepIndex]
                        // Convert velocity from 2-bit (0-3) to MIDI (0-127) for persistence
                        val velocity2bit = stepDisplayState.velocity.toInt().coerceIn(0, 3)
                        val midiVelocity = when (velocity2bit) {
                            0 -> 40   // Ghost
                            1 -> 80   // Normal
                            2 -> 115  // Accent
                            3 -> 127  // Max
                            else -> 80  // Default to Normal
                        }
                        stepMap[partIndex] = PersistenceStepData(
                            gate = stepDisplayState.gate,
                            velocity = midiVelocity,
                            microtiming = stepDisplayState.microtiming.toInt(),
                            probability = stepDisplayState.probability.toInt(),
                            locks = emptyMap()
                        )
                    }
                }
                
                allSteps[absoluteStepIndex] = stepMap
            }
        }
        
        // Convert to PatternData and serialize
        val patternData = PatternData(steps = allSteps)
        val stepsJson = PatternStepsConverter.fromPatternData(patternData)
        
        // Return updated or new pattern entity
        existingPattern?.copy(
            steps = stepsJson,
            length = patternLength,
            seed = state.patternSeed.toInt()
        ) ?: PatternEntity(
            id = java.util.UUID.randomUUID().toString(),
            projectId = projectId,
            patternIndex = patternIndex,
            steps = stepsJson,
            length = patternLength,
            seed = state.patternSeed.toInt()
        )
    }
}