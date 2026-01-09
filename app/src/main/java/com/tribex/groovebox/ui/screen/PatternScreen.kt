package com.tribex.groovebox.ui.screen

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.lifecycle.viewmodel.compose.viewModel
import com.tribex.groovebox.engine.AudioEngineBridge
import com.tribex.groovebox.engine.Velocity
import com.tribex.groovebox.ui.components.FPSCounter
import com.tribex.groovebox.ui.components.StepGrid
import com.tribex.groovebox.ui.components.StickyShiftButton
import com.tribex.groovebox.ui.viewmodel.PatternViewModel
import kotlinx.coroutines.delay
import java.text.SimpleDateFormat
import java.util.*

/**
 * PATTERN Screen
 * 
 * M9: Landscape Layout - Controls left (220dp), StepGrid right (remaining)
 * 16-step grid with paging (16-64)
 * Running light at 60fps
 * Step edit with tap/swipe gestures
 */
@Composable
fun PatternScreen() {
    val context = LocalContext.current
    val viewModel: PatternViewModel = viewModel { PatternViewModel(context) }
    
    // P1.3: Use ViewModel state instead of local state
    val patternState by viewModel.patternState.collectAsState()
    
    var selectedPartIndexInt by remember { mutableStateOf(0) }
    val selectedPartIndex: UInt = selectedPartIndexInt.toUInt()
    var isShiftActive by remember { mutableStateOf(false) }
    var isPlaying by remember { mutableStateOf(false) }
    var isExporting by remember { mutableStateOf(false) }
    var exportProgress by remember { mutableStateOf(0f) }
    
    // Poll state from audio engine (~60fps)
    LaunchedEffect(Unit) {
        while (true) {
            delay(16)  // ~60fps
            
            // Poll audio engine state
            isPlaying = AudioEngineBridge.isPlaying()
            isExporting = AudioEngineBridge.isExporting()
            exportProgress = AudioEngineBridge.getExportProgress()
            
            // P0.5: Update current step from sequencer (audio state, not time-based)
            val currentStep = if (isPlaying) {
                AudioEngineBridge.getCurrentStep().toUInt()
            } else {
                0u
            }
            viewModel.updateCurrentStep(currentStep)
            viewModel.updatePlayingState(isPlaying)
        }
    }
    
    // M9: Landscape Row layout
    Row(
        modifier = Modifier
            .fillMaxSize()
            .padding(8.dp)
    ) {
        // Left Control Column (fixed 220dp width)
        Column(
            modifier = Modifier
                .width(220.dp)
                .fillMaxHeight()
                .padding(end = 8.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            // Header with FPS Counter
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = "PATTERN",
                    style = MaterialTheme.typography.titleMedium
                )
                
                FPSCounter()
            }
            
            HorizontalDivider()
            
            // Part Selection Tabs (compact, horizontal)
            LazyRow(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                items(PARTS.size) { index ->
                    PartTab(
                        part = PARTS[index],
                        isSelected = index.toUInt() == selectedPartIndex,
                        onClick = { selectedPartIndexInt = index }
                    )
                }
            }
            
            // Page Controls (compact)
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedButton(
                    onClick = {
                        if (patternState.currentPage > 0u) {
                            viewModel.updateCurrentPage(patternState.currentPage - 1u)
                        }
                    },
                    enabled = patternState.currentPage > 0u,
                    modifier = Modifier.weight(1f)
                ) {
                    Text("<<")
                }
                
                Text(
                    text = "P${(patternState.currentPage + 1u)}",
                    style = MaterialTheme.typography.titleSmall,
                    modifier = Modifier.padding(horizontal = 4.dp)
                )
                
                OutlinedButton(
                    onClick = {
                        if (patternState.currentPage < 3u) {
                            viewModel.updateCurrentPage(patternState.currentPage + 1u)
                        }
                    },
                    enabled = patternState.currentPage < 3u,
                    modifier = Modifier.weight(1f)
                ) {
                    Text(">>")
                }
            }
            
            // BPM Display
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.Center,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = "${patternState.bpm.toInt()} BPM",
                    style = MaterialTheme.typography.titleLarge
                )
            }
            
            Spacer(modifier = Modifier.height(4.dp))
            
            // Sticky Shift Button
            StickyShiftButton(
                isShiftActive = isShiftActive,
                onShiftToggle = { isActive -> isShiftActive = isActive }
            )
            
            Spacer(modifier = Modifier.height(4.dp))
            
            // Performance Controls (Mute/Solo)
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                OutlinedButton(
                    onClick = { /* Mute functionality - M4 */ },
                    modifier = Modifier.weight(1f)
                ) {
                    Text("MUTE", style = MaterialTheme.typography.labelSmall)
                }
                
                OutlinedButton(
                    onClick = { /* Solo functionality - M4 */ },
                    modifier = Modifier.weight(1f)
                ) {
                    Text("SOLO", style = MaterialTheme.typography.labelSmall)
                }
            }
            
            Spacer(modifier = Modifier.weight(1f))
            
            // Sequencer Controls (Transport)
            Button(
                onClick = {
                    if (isPlaying) {
                        AudioEngineBridge.stopSequencer()
                    } else {
                        AudioEngineBridge.startSequencer()
                    }
                },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text(
                    if (isPlaying) "� STOP" else "� PLAY",
                    style = MaterialTheme.typography.titleMedium
                )
            }
        }
        
        // Right Content Column (flexible width for StepGrid)
        Box(
            modifier = Modifier
                .weight(1f)
                .fillMaxHeight()
        ) {
            StepGrid(
                state = patternState,
                selectedPartIndex = selectedPartIndex,
                onStepTap = { stepIndex ->
                    // P1.3: Use ViewModel to update step (triggers debounced save)
                    val currentGate = patternState.steps
                        .getOrNull(selectedPartIndex.toInt())
                        ?.getOrNull(stepIndex)
                        ?.gate ?: false
                    viewModel.updateStepGate(selectedPartIndex, stepIndex, !currentGate)
                },
                onSwipeUp = { stepIndex ->
                    // P1.3: Set velocity to MAX (accent) via ViewModel
                    viewModel.updateStepVelocity(selectedPartIndex, stepIndex, Velocity.MAX)
                },
                onSwipeDown = { stepIndex ->
                    // P1.3: Set velocity to GHOST via ViewModel
                    viewModel.updateStepVelocity(selectedPartIndex, stepIndex, Velocity.GHOST)
                }
            )
        }
        
        // P1.2: Export Controls
        Spacer(modifier = Modifier.height(8.dp))
        
        if (isExporting) {
            Column(
                modifier = Modifier.fillMaxWidth(),
                verticalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    LinearProgressIndicator(
                        progress = { exportProgress },
                        modifier = Modifier
                            .weight(1f)
                            .height(8.dp)
                    )
                    
                    Button(
                        onClick = {
                            AudioEngineBridge.stopExport()
                        }
                    ) {
                        Text("CANCEL")
                    }
                }
                
                Text(
                    text = "Exporting... ${(exportProgress * 100).toInt()}%",
                    style = MaterialTheme.typography.bodySmall
                )
            }
        } else {
            Button(
                onClick = {
                    // Generate filename with timestamp
                    val timestamp = SimpleDateFormat("yyyyMMdd_HHmmss", Locale.getDefault()).format(Date())
                    val filename = "/data/data/com.tribex.groovebox/files/exports/export_$timestamp.wav"
                    AudioEngineBridge.startExport(filename)
                },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("EXPORT WAV")
            }
        }
    }
}

/**
 * Part Tab
 * 
 * Part selection button (one of 9 parts) - Compact version for landscape
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun PartTab(
    part: PartInfo,
    isSelected: Boolean,
    onClick: () -> Unit
) {
    val backgroundColor = if (isSelected) {
        MaterialTheme.colorScheme.primary
    } else {
        MaterialTheme.colorScheme.surfaceVariant
    }
    
    val textColor = if (isSelected) {
        MaterialTheme.colorScheme.onPrimary
    } else {
        MaterialTheme.colorScheme.onSurfaceVariant
    }
    
    FilterChip(
        selected = isSelected,
        onClick = onClick,
        label = { Text(part.name, style = MaterialTheme.typography.labelSmall) },
        colors = FilterChipDefaults.filterChipColors(
            selectedContainerColor = backgroundColor,
            selectedLabelColor = textColor,
            containerColor = backgroundColor,
            labelColor = textColor
        )
    )
}
