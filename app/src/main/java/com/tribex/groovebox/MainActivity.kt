package com.tribex.groovebox

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import com.tribex.groovebox.engine.AudioEngineBridge
import com.tribex.groovebox.ui.screen.Screen
import com.tribex.groovebox.ui.screen.PatternScreen
import com.tribex.groovebox.ui.screen.SoundScreen
import com.tribex.groovebox.ui.SampleScreen
import com.tribex.groovebox.ui.components.NavigationBar
import com.tribex.groovebox.ui.theme.TribexTheme
import com.tribex.groovebox.engine.PartSampleState
import com.tribex.groovebox.engine.SampleImportResult
import com.tribex.groovebox.engine.VoiceParams

/**
 * MainActivity - TribeX M3: PATTERN Screen + Performance UI
 * 
 * 3 Screens: PATTERN / SOUND / SAMPLE
 * Navigation via bottom bar
 * Per SPEC v3.1: No sub-menus, exactly 3 screens
 */
class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
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
}

/**
 * Main Navigation
 * 
 * Bottom navigation bar with 3 screens
 * Content switches based on selected screen
 * 
 * M4: Added sample state management for SAMPLE screen
 */
@Composable
fun MainNavigation() {
    var currentScreen by remember { mutableStateOf(Screen.PATTERN) }
    
    // M4: Sample state management
    var parts by remember { mutableStateOf(PartSampleState.createEmpty()) }
    
    Scaffold(
        bottomBar = {
            NavigationBar(
                currentScreen = currentScreen,
                onScreenChange = { currentScreen = it }
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