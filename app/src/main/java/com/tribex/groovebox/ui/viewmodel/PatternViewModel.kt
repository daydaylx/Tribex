package com.tribex.groovebox.ui.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.tribex.groovebox.engine.Velocity
import com.tribex.groovebox.persistence.PatternStepsConverter
import com.tribex.groovebox.persistence.ProjectManager
import com.tribex.groovebox.persistence.StepData as PersistenceStepData
import com.tribex.groovebox.persistence.PatternData
import com.tribex.groovebox.persistence.entities.Pattern as PatternEntity
import com.tribex.groovebox.ui.screen.PatternState
import com.tribex.groovebox.ui.screen.StepDisplayState
import com.tribex.groovebox.engine.AudioEngineBridge
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import android.content.Context

/**
 * PatternViewModel - Manages Pattern state with Room persistence
 * 
 * P1.3: UI ↔ Room Integration
 * - Loads pattern from Room on init
 * - Saves pattern to Room on step changes (debounced)
 * - Converts between PatternState (UI) and Pattern (Entity)
 */
class PatternViewModel(context: Context) : ViewModel() {
    
    private val projectManager = ProjectManager.getInstance(context)
    
    // UI State
    private val _patternState = MutableStateFlow<PatternState>(PatternState.createEmpty())
    val patternState: StateFlow<PatternState> = _patternState.asStateFlow()
    
    // Current pattern index (0-3)
    private var currentPatternIndex: Int = 0
    
    // Debounce job for saving
    private var saveJob: Job? = null
    
    // Save debounce delay (ms)
    private val SAVE_DEBOUNCE_MS = 500L
    
    init {
        // Load pattern on init
        loadPattern(0)
    }
    
    /**
     * Load pattern from Room
     * 
     * @param patternIndex Pattern index (0-3)
     */
    fun loadPattern(patternIndex: Int) {
        currentPatternIndex = patternIndex
        
        viewModelScope.launch(Dispatchers.IO) {
            val project = projectManager.getCurrentProject()
            val patternDao = projectManager.getPatternDao()
            
            if (project != null && patternDao != null) {
                val patternEntity = patternDao.getByProjectAndIndex(project.id, patternIndex)
                
                if (patternEntity != null) {
                    // Convert Pattern Entity to PatternState
                    val patternState = convertEntityToState(patternEntity)
                    withContext(Dispatchers.Main) {
                        // Preserve current page if within bounds
                        val currentPage = _patternState.value.currentPage
                        val maxPage = ((patternState.patternLengthSteps.toInt() - 1) / 16).toUInt()
                        val preservedPage = if (currentPage <= maxPage) currentPage else 0u
                        
                        _patternState.value = patternState.copy(currentPage = preservedPage)
                        
                        // Set BPM in audio engine
                        AudioEngineBridge.setBPM(patternState.bpm)
                        
                        // Send pattern to audio engine
                        sendPatternToAudioEngine(patternState)
                    }
                } else {
                    // Pattern doesn't exist, use empty state
                    val projectBpm = project.bpm
                    withContext(Dispatchers.Main) {
                        val emptyPatternState = PatternState.createEmpty().copy(
                            currentPatternId = patternIndex.toUInt(),
                            bpm = projectBpm
                        )
                        _patternState.value = emptyPatternState
                        
                        // Set BPM in audio engine
                        AudioEngineBridge.setBPM(emptyPatternState.bpm)
                        
                        // Send empty pattern to audio engine so sequencer has valid pattern data
                        sendPatternToAudioEngine(emptyPatternState)
                    }
                }
            }
        }
    }
    
    /**
     * Update step gate (triggers debounced save)
     */
    fun updateStepGate(partIndex: UInt, stepIndex: Int, gate: Boolean) {
        val currentState = _patternState.value
        val partSteps = currentState.steps.getOrNull(partIndex.toInt())
        
        if (partSteps != null && stepIndex < partSteps.size) {
            val newSteps = partSteps.toMutableList()
            newSteps[stepIndex] = newSteps[stepIndex].copy(gate = gate)
            
            val allParts = currentState.steps.toMutableList()
            allParts[partIndex.toInt()] = newSteps
            
            _patternState.value = currentState.copy(steps = allParts)
            
            // Debounced save
            scheduleSave()
            
            // Send updated pattern to audio engine immediately
            sendPatternToAudioEngine(_patternState.value)
        }
    }
    
    /**
     * Update step velocity (triggers debounced save)
     */
    fun updateStepVelocity(partIndex: UInt, stepIndex: Int, velocity: UByte) {
        val currentState = _patternState.value
        val partSteps = currentState.steps.getOrNull(partIndex.toInt())
        
        if (partSteps != null && stepIndex < partSteps.size) {
            val newSteps = partSteps.toMutableList()
            newSteps[stepIndex] = newSteps[stepIndex].copy(velocity = velocity)
            
            val allParts = currentState.steps.toMutableList()
            allParts[partIndex.toInt()] = newSteps
            
            _patternState.value = currentState.copy(steps = allParts)
            
            // Debounced save
            scheduleSave()
            
            // Send updated pattern to audio engine immediately
            sendPatternToAudioEngine(_patternState.value)
        }
    }
    
    /**
     * Update current step (for running light)
     */
    fun updateCurrentStep(step: UInt) {
        _patternState.value = _patternState.value.copy(currentStep = step)
    }
    
    /**
     * Update playing state
     */
    fun updatePlayingState(isPlaying: Boolean) {
        _patternState.value = _patternState.value.copy(isPlaying = isPlaying)
    }
    
    /**
     * Update BPM
     */
    fun updateBPM(bpm: Float) {
        _patternState.value = _patternState.value.copy(bpm = bpm)
    }
    
    /**
     * Update current page
     * Reloads pattern data for the new page
     */
    fun updateCurrentPage(page: UInt) {
        val currentState = _patternState.value
        val patternLength = currentState.patternLengthSteps.toInt()
        val maxPage = ((patternLength - 1) / 16).toUInt()
        
        if (page <= maxPage) {
            _patternState.value = currentState.copy(currentPage = page)
            
            // Reload pattern to get correct page data
            loadPattern(currentPatternIndex)
        }
    }
    
    /**
     * Update pattern seed
     */
    fun updatePatternSeed(seed: UInt) {
        _patternState.value = _patternState.value.copy(patternSeed = seed)
        scheduleSave()
    }
    
    /**
     * Update step microtiming (triggers debounced save)
     */
    fun updateStepMicrotiming(partIndex: UInt, stepIndex: Int, microtiming: Byte) {
        val currentState = _patternState.value
        val partSteps = currentState.steps.getOrNull(partIndex.toInt())
        
        if (partSteps != null && stepIndex < partSteps.size) {
            val newSteps = partSteps.toMutableList()
            newSteps[stepIndex] = newSteps[stepIndex].copy(
                microtiming = microtiming.coerceIn(-50, 50)
            )
            
            val allParts = currentState.steps.toMutableList()
            allParts[partIndex.toInt()] = newSteps
            
            _patternState.value = currentState.copy(steps = allParts)
            
            // Debounced save
            scheduleSave()
            
            // Send updated pattern to audio engine immediately
            sendPatternToAudioEngine(_patternState.value)
        }
    }
    
    /**
     * Schedule debounced save
     */
    private fun scheduleSave() {
        saveJob?.cancel()
        saveJob = viewModelScope.launch(Dispatchers.IO) {
            delay(SAVE_DEBOUNCE_MS)
            savePattern()
        }
    }
    
    /**
     * Save pattern to Room
     */
    private suspend fun savePattern() {
        val project = projectManager.getCurrentProject()
        val patternDao = projectManager.getPatternDao()
        
        if (project != null && patternDao != null) {
            val currentState = _patternState.value
            
            // Convert PatternState to Pattern Entity
            val patternEntity = convertStateToEntity(
                currentState,
                project.id,
                currentPatternIndex
            )
            
            // Check if pattern exists
            val existingPattern = patternDao.getByProjectAndIndex(project.id, currentPatternIndex)
            
            if (existingPattern != null) {
                patternDao.update(patternEntity)
            } else {
                patternDao.insert(patternEntity)
            }
        }
    }
    
    /**
     * Convert Pattern Entity to PatternState
     */
    private suspend fun convertEntityToState(entity: PatternEntity): PatternState = withContext(Dispatchers.IO) {
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
        val currentPage = withContext(Dispatchers.Main) {
            _patternState.value.currentPage
        }
        
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
     * Handles multi-page patterns (16/32/48/64 steps)
     * Merges current page changes with existing pattern data
     */
    private suspend fun convertStateToEntity(
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
        
        // Count active steps in allSteps for debugging
        var activeStepsInAllSteps = 0
        for (stepIndex in 0 until patternLength) {
            val stepMap = allSteps.getOrNull(stepIndex)
            if (stepMap != null) {
                for (partIndex in 0 until 9) {
                    val stepData = stepMap[partIndex]
                    if (stepData?.gate == true) {
                        activeStepsInAllSteps++
                    }
                }
            }
        }
        android.util.Log.d("PatternViewModel", "convertStateToEntity: patternLength=$patternLength, allSteps.size=${allSteps.size}, activeStepsInAllSteps=$activeStepsInAllSteps, currentPage=$currentPage")
        
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
    
    /**
     * Send pattern data to audio engine
     * Serializes PatternState to C++ Pattern structure format
     * 
     * Uses PatternState directly (includes unsaved changes) by converting to full pattern entity first
     */
    private fun sendPatternToAudioEngine(state: PatternState) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                val project = projectManager.getCurrentProject()
                
                if (project != null) {
                    val patternLength = state.patternLengthSteps.toInt()
                    
                    if (patternLength > 0) {
                        // Convert PatternState to full PatternEntity (includes all steps, not just current page)
                        val patternEntity = convertStateToEntity(state, project.id, currentPatternIndex)
                        val patternData = PatternStepsConverter.toPatternData(patternEntity.steps)
                        
                        // Serialize pattern: C++ Pattern structure is:
                        // uint32_t id (4 bytes)
                        // uint32_t lengthSteps (4 bytes)
                        // uint32_t patternSeed (4 bytes)
                        // StepData steps[NUM_PARTS][MAX_STEPS] where StepData is:
                        //   uint8_t gate (1 byte)
                        //   uint8_t velocity (1 byte) - 2-bit value (0-3)
                        //   int8_t microtiming (1 byte)
                        //   int8_t probability (1 byte)
                        
                        // Calculate serialized size: header (12) + steps (NUM_PARTS * MAX_STEPS * 4 bytes per step)
                        val NUM_PARTS = 9
                        val MAX_STEPS = 64
                        val headerSize = 12 // id(4) + lengthSteps(4) + patternSeed(4)
                        val stepSize = 4 // gate(1) + velocity(1) + microtiming(1) + probability(1)
                        val totalSize = headerSize + (NUM_PARTS * MAX_STEPS * stepSize)
                        
                        val byteArray = ByteArray(totalSize)
                        var offset = 0
                        
                        // Write header
                        writeUInt32(byteArray, offset, patternEntity.patternIndex)
                        offset += 4
                        writeUInt32(byteArray, offset, patternLength)
                        offset += 4
                        writeUInt32(byteArray, offset, patternEntity.seed)
                        offset += 4
                        
                        // Write steps: iterate all parts and all steps
                        for (partIndex in 0 until NUM_PARTS) {
                            for (stepIndex in 0 until MAX_STEPS) {
                                val stepData = if (stepIndex < patternLength) {
                                    patternData.steps.getOrNull(stepIndex)?.get(partIndex)
                                } else {
                                    null
                                }
                                
                                // Write StepData: gate, velocity, microtiming, probability
                                // Velocity in persistence is stored as Int (0-127 MIDI), but PatternState uses UByte (0-3)
                                // When loaded from Room, velocity is converted from MIDI to 2-bit in convertEntityToState
                                // When saving to Room, velocity is converted from 2-bit to MIDI in convertStateToEntity
                                // So stepData.velocity here is MIDI (0-127), but we need 2-bit (0-3)
                                val midiVelocity = stepData?.velocity ?: 80
                                val velocity2bit = when {
                                    midiVelocity < 45 -> 0u  // Ghost
                                    midiVelocity < 75 -> 1u  // Normal
                                    midiVelocity < 95 -> 2u  // Accent
                                    else -> 3u  // Max
                                }
                                
                                byteArray[offset++] = if (stepData?.gate == true) 1u.toByte() else 0u.toByte()
                                byteArray[offset++] = velocity2bit.toByte()
                                byteArray[offset++] = (stepData?.microtiming ?: 0).coerceIn(-50, 50).toByte()
                                byteArray[offset++] = (stepData?.probability ?: 100).coerceIn(0, 100).toByte()
                            }
                        }
                        
                        // Count active steps for debugging (BEFORE sending)
                        var activeSteps = 0
                        for (partIndex in 0 until NUM_PARTS) {
                            for (stepIndex in 0 until patternLength) {
                                val stepData = patternData.steps.getOrNull(stepIndex)?.get(partIndex)
                                if (stepData?.gate == true) {
                                    activeSteps++
                                }
                            }
                        }
                        
                        // Count active steps in serialized byte array (AFTER serialization)
                        var serializedActiveSteps = 0
                        var serializedOffset = headerSize
                        for (partIndex in 0 until NUM_PARTS) {
                            for (stepIndex in 0 until MAX_STEPS) {
                                if (stepIndex < patternLength && serializedOffset + stepSize <= byteArray.size) {
                                    val gate = byteArray[serializedOffset] != 0.toByte()
                                    if (gate) {
                                        serializedActiveSteps++
                                    }
                                }
                                serializedOffset += stepSize
                            }
                        }
                        
                        android.util.Log.d("PatternViewModel", "Pattern sent to audio engine: length=$patternLength, seed=${patternEntity.seed}, dataSize=${byteArray.size}, activeStepsInPatternData=$activeSteps, activeStepsInSerialized=$serializedActiveSteps, patternDataStepsSize=${patternData.steps.size}")
                        
                        // Send to audio engine
                        AudioEngineBridge.setPattern(byteArray, patternLength, patternEntity.seed)
                    } else {
                        android.util.Log.w("PatternViewModel", "Pattern length is 0, not sending to audio engine")
                    }
                } else {
                    android.util.Log.w("PatternViewModel", "Project is null, cannot send pattern to audio engine")
                }
            } catch (e: Exception) {
                // Log error but don't crash
                android.util.Log.e("PatternViewModel", "Failed to send pattern to audio engine", e)
                e.printStackTrace()
            }
        }
    }
    
    /**
     * Write uint32_t to byte array (little-endian)
     */
    private fun writeUInt32(array: ByteArray, offset: Int, value: Int) {
        array[offset] = (value and 0xFF).toByte()
        array[offset + 1] = ((value shr 8) and 0xFF).toByte()
        array[offset + 2] = ((value shr 16) and 0xFF).toByte()
        array[offset + 3] = ((value shr 24) and 0xFF).toByte()
    }
}
