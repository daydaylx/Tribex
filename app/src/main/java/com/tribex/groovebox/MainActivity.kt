package com.tribex.groovebox

import android.os.Bundle
import android.util.Log
import java.io.File
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.ProcessLifecycleOwner
import com.tribex.groovebox.engine.AudioEngineBridge
import com.tribex.groovebox.persistence.ProjectManager
import com.tribex.groovebox.ui.screen.Screen
import com.tribex.groovebox.ui.screen.PatternScreen
import com.tribex.groovebox.ui.screen.SoundScreen
import com.tribex.groovebox.ui.SampleScreen
import com.tribex.groovebox.ui.components.NavigationBar
import com.tribex.groovebox.ui.theme.TribexTheme
import com.tribex.groovebox.engine.PartSampleState
import com.tribex.groovebox.engine.SampleImportResult
import com.tribex.groovebox.engine.VoiceParams
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch

/**
 * MainActivity - TribeX M3: PATTERN Screen + Performance UI
 * 
 * 3 Screens: PATTERN / SOUND / SAMPLE
 * Navigation via bottom bar
 * Per SPEC v3.1: No sub-menus, exactly 3 screens
 * 
 * M8: Added autosave on app pause/background
 */
class MainActivity : ComponentActivity() {
    
    private val TAG = "MainActivity"
    private val coroutineScope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
    private lateinit var projectManager: ProjectManager
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Initialize ProjectManager
        projectManager = ProjectManager.getInstance(this)
        
        // Set debug log path for native code
        val logFile = File(filesDir, "debug.log")
        AudioEngineBridge.setDebugLogPath(logFile.absolutePath)
        
        // M8: Setup autosave on app pause/background
        setupAutosave()
        
        setContent {
            TribexTheme {
                MainNavigation()
            }
        }
    }
    
    override fun onDestroy() {
        super.onDestroy()
        // Cleanup native audio engine
        AudioEngineBridge.cleanup()
    }
    
    /**
     * M8: Setup autosave triggers
     * - App goes to background (ProcessLifecycleOwner.ON_STOP)
     * - App is paused (Lifecycle.Event.ON_PAUSE)
     */
    private fun setupAutosave() {
        // App background trigger
        val processObserver = LifecycleEventObserver { _, event ->
            if (event == Lifecycle.Event.ON_STOP) {
                Log.d(TAG, "App went to background - triggering autosave")
                coroutineScope.launch(Dispatchers.IO) {
                    projectManager.autosave()
                        .onFailure { e ->
                            Log.e(TAG, "Autosave failed", e)
                        }
                        .onSuccess {
                            Log.d(TAG, "Autosave successful")
                        }
                }
            }
        }
        ProcessLifecycleOwner.get().lifecycle.addObserver(processObserver)
        
        // App pause trigger
        val activityObserver = LifecycleEventObserver { _, event ->
            if (event == Lifecycle.Event.ON_PAUSE) {
                Log.d(TAG, "App paused - triggering autosave")
                coroutineScope.launch(Dispatchers.IO) {
                    projectManager.autosave()
                        .onFailure { e ->
                            Log.e(TAG, "Autosave failed", e)
                        }
                        .onSuccess {
                            Log.d(TAG, "Autosave successful")
                        }
                }
            }
        }
        lifecycle.addObserver(activityObserver)
    }
}

/**
 * Main Navigation
 * 
 * Bottom navigation bar with 3 screens
 * Content switches based on selected screen
 * 
 * M4: Added sample state management for SAMPLE screen
 * M8: Added autosave on screen change
 */
@Composable
fun MainNavigation() {
    var currentScreen by remember { mutableStateOf(Screen.PATTERN) }
    val projectManager = ProjectManager.getInstance(LocalContext.current)
    val coroutineScope = rememberCoroutineScope()
    
    // M4: Sample state management
    var parts by remember { mutableStateOf(PartSampleState.createEmpty()) }
    
    // P1.3: Ensure project is loaded before showing PatternScreen
    LaunchedEffect(Unit) {
        val project = projectManager.getCurrentProject()
        if (project == null) {
            // Try to load last project, or create new one
            projectManager.loadLastProject()
                .onFailure { e ->
                    android.util.Log.e("MainNavigation", "Failed to load last project", e)
                    // Create new project if load fails
                    projectManager.createProject("Untitled Project")
                        .onFailure { e2 ->
                            android.util.Log.e("MainNavigation", "Failed to create project", e2)
                        }
                }
        }
    }
    
    Scaffold(
        bottomBar = {
            NavigationBar(
                currentScreen = currentScreen,
                onScreenChange = { newScreen ->
                    // M8: Autosave on screen change
                    coroutineScope.launch(Dispatchers.IO) {
                        projectManager.autosave()
                            .onFailure { e ->
                                android.util.Log.e("MainNavigation", "Autosave failed", e)
                            }
                            .onSuccess {
                                android.util.Log.d("MainNavigation", "Autosave successful on screen change")
                            }
                    }
                    currentScreen = newScreen
                }
            )
        }
    ) { paddingValues ->
        when (currentScreen) {
            Screen.PATTERN -> PatternScreen()
            Screen.SOUND -> SoundScreen()
            Screen.SAMPLE -> {
                SampleScreen(
                    parts = parts,
                    onSampleLoaded = { partIndex, result ->
                        val emptyPart = parts.firstOrNull { !it.hasSample }
                        if (emptyPart != null && result.metadata != null && result.sampleData != null) {
                            val targetPartIndex = if (partIndex == -1) emptyPart.partIndex else partIndex
                            
                            // Load sample into audio engine
                            AudioEngineBridge.loadSample(
                                targetPartIndex,
                                result.sampleData,
                                result.sampleData.size,
                                result.metadata.sampleRate,
                                result.sampleId,
                                result.metadata.startOffset,
                                result.metadata.endOffset
                            )
                            
                            val updatedParts = parts.mapIndexed { idx, part ->
                                if (idx == targetPartIndex) {
                                    part.copy(
                                        sampleId = result.sampleId,
                                        metadata = result.metadata,
                                        hasSample = true
                                    )
                                } else {
                                    part
                                }
                            }
                            parts = updatedParts
                        }
                    },
                    onVoiceParamChanged = { partIndex, params ->
                        val updatedParts = parts.mapIndexed { idx, part ->
                            if (idx == partIndex) {
                                part.copy(params = params)
                            } else {
                                part
                            }
                        }
                        parts = updatedParts
                        
                        AudioEngineBridge.setVoicePitch(partIndex, params.pitch)
                        AudioEngineBridge.setVoicePan(partIndex, params.pan)
                        AudioEngineBridge.setVoiceLevel(partIndex, params.level)
                        AudioEngineBridge.setVoiceDecay(partIndex, params.decayMs)
                        AudioEngineBridge.setVoiceFilter(partIndex, params.filter.id)
                    },
                    onPartMuteChanged = { partIndex, muted ->
                        val updatedParts = parts.mapIndexed { idx, part ->
                            if (idx == partIndex) {
                                part.copy(muted = muted)
                            } else {
                                part
                            }
                        }
                        parts = updatedParts
                        AudioEngineBridge.setPartMute(partIndex, muted)
                    },
                    onPartSoloChanged = { partIndex, soloed ->
                        val updatedParts = parts.mapIndexed { idx, part ->
                            if (idx == partIndex) {
                                part.copy(soloed = soloed)
                            } else {
                                part
                            }
                        }
                        parts = updatedParts
                        AudioEngineBridge.setPartSolo(partIndex, soloed)
                    },
                    modifier = Modifier.padding(paddingValues)
                )
            }
        }
    }
}