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
 *
 * This ViewModel manages the state of a single pattern within the TribeX groovebox.
 * It handles:
 * - Loading and saving patterns from/to Room database
 * - Managing pattern state (steps, BPM, etc.)
 * - Converting between UI state and persistence format
 * - Debounced saving to avoid excessive database writes
 * - Communication with the audio engine
 *
 * @property context Android context used to access ProjectManager and other services
 */
class PatternViewModel(context: Context) : ViewModel() {
    
    private val patternConverter = PatternConverter(context)
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
     * Loads a pattern from the Room database and updates the UI state.
     * If the pattern doesn't exist, creates an empty pattern with the project's BPM.
     * Also sends the pattern to the audio engine for playback.
     *
     * @param patternIndex Pattern index (0-3) within the current project
     */
    fun loadPattern(patternIndex: Int) {
        currentPatternIndex = patternIndex
        
        viewModelScope.launch(Dispatchers.IO) {
            val project = projectManager.getCurrentProject()
            val patternDao = projectManager.getPatternDao()
            
            if (project != null && patternDao != null) {
                val patternEntity = patternDao.getByProjectAndIndex(project.id, patternIndex)
                
                if (patternEntity != null) {
                    // Convert Pattern Entity to PatternState using PatternConverter
                    val patternState = patternConverter.convertEntityToState(patternEntity)
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
            
            // Convert PatternState to Pattern Entity using PatternConverter
            val patternEntity = patternConverter.convertStateToEntity(
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
                        val patternEntity = patternConverter.convertStateToEntity(state, project.id, currentPatternIndex)
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
